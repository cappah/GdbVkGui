#include "ProcessIO.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
		struct timespec slp_period = { .tv_nsec = SecToNano(secs*0.1) };
		nanosleep(&slp_period, NULL);
    }
}

//-----------------------------------------------------------------------------

static char s_gdb_out[1024 * 8];
static char s_cmd_buff[1024];
static char s_err[1024];

static int s_frontend_to_gdb[2];
static int s_gdb_to_frontend[2];

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
	}
	else if (written >= (int)sizeof(s_err)) {
		snprintf(s_err, sizeof(s_err), "Command exceeds buffer {%d > %d}", written, (int)sizeof(s_err));

		return false;
	}

	write(s_frontend_to_gdb[1], s_cmd_buff, written);

	// give gdb time to respond
	TimedWait(0.01);

	return true;
}

GdbMsg
GdbOutput(void)
{
	// Give gdb time to respond & send message
	s_gdb_out[0] = 0;

	uint32_t msg_sz = 0;
	char buff[256] = { 0 };

	int read_bytes = 1;
	double timeout = 0;
	while(read_bytes && (timeout < 0.07)) {
		read_bytes = read(s_gdb_to_frontend[0], buff, sizeof(buff));

		// gdb sends data
		if (read_bytes > 0) { 
			memcpy(s_gdb_out + msg_sz, buff, read_bytes);
			msg_sz += read_bytes;
		}
		// wait on non-blocking socket to not read bad data
		else if ((read_bytes == -1) && (errno == EAGAIN)) {
			double wait_t = 0.03;
			TimedWait(wait_t);
			timeout += wait_t;
		}

		// Safety net. Shouldn't be hit, unless to data from gdb is huge
		if (msg_sz == (sizeof(s_gdb_out) - 1)) {
			read_bytes = 0;
			s_gdb_out[msg_sz] = 0;
		}
	}

	GdbMsg output = { .m_MsgSz = msg_sz, .m_Msg = s_gdb_out };
	return output;
}

