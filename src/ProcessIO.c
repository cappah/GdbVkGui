#include "ProcessIO.h"
#include "tlsf.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

uint64_t
GetHighResTime(void)
{
    uint64_t        hrTime = 0;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    hrTime = (now.tv_sec * 1000000000L) + now.tv_nsec;

    return hrTime;
}

long int
SecToNano(double seconds)
{
    if (seconds < 0.0f) {
        return 0;
    }

    double nanosecs = seconds * 1000000000.0;
    return (uint64_t)nanosecs;
}

double
NanoToSec(uint64_t nanosecs)
{
    return (double)nanosecs / 1000000000.0;
}

void
TimedWait(double secs)
{
    double start_time = NanoToSec(GetHighResTime());

    while ((start_time + secs) > NanoToSec(GetHighResTime())) {
        struct timespec slp_period = { .tv_nsec = SecToNano(secs * 0.1) };
        nanosleep(&slp_period, NULL);
    }
}

//-----------------------------------------------------------------------------

static char s_gdb_out[(0x1 << 20) * 10]; // 10 megabyte buffer
static char s_cmd_buff[1024];
static char s_err[1024];

static int s_frontend_to_gdb[2];
static int s_gdb_to_frontend[2];

static tlsf_t s_heap;
static pool_t s_pool;

//-----------------------------------------------------------------------------

void
InitMemoryArena(size_t mem_alloc_sz)
{
    if (s_heap) {
        tlsf_destroy(s_heap);
    }
    s_pool = malloc(mem_alloc_sz);
    assert(s_pool && "Failed to initialize memory arena");

    s_heap = tlsf_create_with_pool(s_pool, mem_alloc_sz);
}

void*
WmMalloc(size_t sz)
{
    return tlsf_malloc(s_heap, sz);
}

void*
WmRealloc(void* ptr, size_t sz)
{
    return tlsf_realloc(s_heap, ptr, sz);
}

void
WmFree(void* ptr)
{
    tlsf_free(s_heap, ptr);
}

//-----------------------------------------------------------------------------

int*
GetFtoGPipes(void)
{
    return s_frontend_to_gdb;
}

int*
GetGtoFPipes(void)
{
    return s_gdb_to_frontend;
}

bool
SendCommand(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(s_cmd_buff, sizeof(s_cmd_buff), fmt, args);
    va_end(args);

    if (written < 0) {
        char err_msg[256] = { 0 };
        strerror_r(errno, err_msg, sizeof(err_msg));
        snprintf(s_err, sizeof(s_err), "Failed to parse command: %s", err_msg);

        return false;
    } else if (written >= (int)sizeof(s_err)) {
        snprintf(s_err,
                 sizeof(s_err),
                 "Command exceeds buffer {%d > %d}",
                 written,
                 (int)sizeof(s_err));

        return false;
    }

    write(s_frontend_to_gdb[1], s_cmd_buff, written);

    return true;
}

GdbMsg
GdbOutput(void)
{
    // Give gdb time to respond & send message
    s_gdb_out[0] = 0;

    uint32_t msg_sz    = 0;
    char     buff[256] = { 0 };

    int    read_bytes = 1;
    double timeout    = 0;
    while (read_bytes && (timeout < 0.05)) {
        read_bytes = read(s_gdb_to_frontend[0], buff, sizeof(buff));

        // gdb sends data
        if (read_bytes > 0 && (msg_sz + read_bytes < sizeof(s_gdb_out))) {
            memcpy(s_gdb_out + msg_sz, buff, read_bytes);
            msg_sz += read_bytes;
        }
        // wait on non-blocking socket to not read bad data
        else if ((read_bytes == -1) && (errno == EAGAIN)) {
            double wait_t = 0.005;
            TimedWait(wait_t);
            timeout += wait_t;
        } else {
            break;
        }

        // Safety net. Shouldn't be hit, unless to data from gdb is huge
        // if (msg_sz == (sizeof(s_gdb_out) - 1)) {
        //    read_bytes        = 0;
        //    s_gdb_out[msg_sz] = 0;
        //}
    }

    GdbMsg output = { .m_MsgSz = msg_sz, .m_Msg = s_gdb_out };
    return output;
}

//-----------------------------------------------------------------------------

bool
GetFileInfo(const char* fname, FileInfo* f_info)
{
    struct stat inf = { 0 };
    if (stat(fname, &inf) != 0) {
        return false;
    }

    memset(f_info->m_Name, 0, sizeof(f_info->m_Name));
    memcpy(f_info->m_Name, fname, strlen(fname));

    f_info->m_Sz         = (uint32_t)inf.st_size;
    f_info->m_LastAccess = (uint32_t)inf.st_atime;
    f_info->m_LastEdit   = (uint32_t)inf.st_mtime;
    f_info->m_LastChange = (uint32_t)inf.st_ctime;

    return true;
}

bool
ReadFile(const char* fname, FileInfo* f_info)
{
    if (GetFileInfo(fname, f_info)) {
        int fd = open(fname, O_RDONLY);
        if (fd != -1) {
            memset(f_info->m_Contents, 0, f_info->m_ContentMaxSz);
            if (!f_info->m_Contents) {
                close(fd);
                return false;
            }

            // TODO : Handles case where file sz is too large?

            int sts = read(fd, f_info->m_Contents, f_info->m_Sz);
            (void)sts;

            close(fd);
            return true;
        }
    }
    return false;
}
