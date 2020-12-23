#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "Frontend/GdbFE.h"
#include "Gui/GuiLayer.h"
#include "LuaLayer.h"
#include "ProcessIO.h"
#include "UtilityMacros.h"
#include "Vulkan/VulkanLayer.h"
#include "WindowInterface.h"

extern const char _binary__tmp_imguisettings_ini_start;
extern const char _binary__tmp_imguisettings_ini_end;
extern const char _binary__tmp_prog_luac_start;
extern const char _binary__tmp_prog_luac_end;

static void
PrintErr(const char* err_stdout)
{
    perror(err_stdout);
    _exit(1);
}

static pid_t
CreateGdbProcess(const char* g_exe, int* f_to_g, int* g_to_f)
{
    pid_t gdb_process = fork();
    if (gdb_process == -1) {
        PrintErr("Failed to create gdb child process: ");
    }

    if (gdb_process == 0) { // gdb child process
        // Connect gdb ends to standard input and output pipes
        dup2(f_to_g[0], STDIN_FILENO);
        dup2(g_to_f[1], STDOUT_FILENO);

        char* gdb_argv[4] = {
            (char*)g_exe, (char*)"--interpreter=mi", (char*)"--quiet", NULL
        };
        execvp(gdb_argv[0], gdb_argv);

        _exit(0);
    }
    return gdb_process;
}

static void
CreateUserDir(void)
{
    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        int wout = write(STDERR_FILENO, "\nFailed to locate HOME.\n", 25);
        UNUSED_VAR(wout);
        _exit(3);
    }

    int  home_len       = strlen(home_dir);
    char filename[1024] = { 0 };
    memcpy(filename, home_dir, home_len);

    const char* dir_name = "/.gdbvkgui";
    int         dir_len  = strlen(dir_name);

    // create directory
    memcpy(filename + home_len, dir_name, dir_len);
    mkdir(filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    const char* files_to_make[2] = {
        "/.gdbvkgui/imgui.ini",
        "/.gdbvkgui/prog.luac",
    };
    const char* data_to_dump[2] = {
        &_binary__tmp_imguisettings_ini_start,
        &_binary__tmp_prog_luac_start,
    };
    int data_sz[2] = {
        (int)(&_binary__tmp_imguisettings_ini_end -
              &_binary__tmp_imguisettings_ini_start),
        (int)(&_binary__tmp_prog_luac_end - &_binary__tmp_prog_luac_start),
    };

    for (int i = 0; i < 2; i++) {
        FileInfo f_info = { 0 };
        memset(filename, 0, sizeof(filename));

        memcpy(filename, home_dir, home_len);
        memcpy(filename + home_len, files_to_make[i], strlen(files_to_make[i]));

        if (GetFileInfo(filename, &f_info) == false) {
            int fd = open(filename,
                          O_WRONLY | O_CREAT,
                          S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            // dump default contents
            if (fd != -1) {
                int wout = write(fd, data_to_dump[i], data_sz[i]);
                UNUSED_VAR(wout);

                close(fd);
            } else {
                PrintErr("Failed to create default files");
            }
        }
    }
}

int
main(const int argc, const char* argv[])
{
    // Make sure gdb is an exe that exists
    const char* gdb_exe = "/usr/bin/gdb";
    if (access(gdb_exe, X_OK)) {
        PrintErr("Failed to find gdb exe: ");
    }

    CreateUserDir();

    // Open non-blocking pipes for inter-process communication
    // [0] : read end
    // [1] : write end
    int* fd_frontend_to_gdb = GetFtoGPipes();
    int* fd_gdb_to_frontend = GetGtoFPipes();

    if (pipe(fd_frontend_to_gdb) || pipe(fd_gdb_to_frontend)) {
        PrintErr("Failed to open pipes for gdb communication: ");
    }

    // initialize memory
    InitMemoryArena((0x1 << 20) * 50); // 50 mb

    // initialize lua
    InitLuaState();

    // frontend
    int fstate = fcntl(fd_frontend_to_gdb[1], F_GETFL, 0);
    fstate     = fstate | O_NONBLOCK;
    fcntl(fd_frontend_to_gdb[1], F_SETFL, fstate);

    fstate = fcntl(fd_gdb_to_frontend[0], F_GETFL, 0);
    fstate = fstate | O_NONBLOCK;
    fcntl(fd_gdb_to_frontend[0], F_SETFL, fstate);

    // gdb
    fstate = fcntl(fd_frontend_to_gdb[0], F_GETFL, 0);
    fstate = fstate | O_NONBLOCK;
    fcntl(fd_frontend_to_gdb[0], F_SETFL, fstate);

    // fstate = fcntl(fd_gdb_to_frontend[1], F_GETFL, 0);
    // fstate = fstate | O_NONBLOCK;
    // fcntl(fd_gdb_to_frontend[1], F_SETFL, fstate);

    pid_t gdb_process =
      CreateGdbProcess(gdb_exe, fd_frontend_to_gdb, fd_gdb_to_frontend);

    // if (SendCommand("-gdb-version")) {
    //    GdbMsg out = GdbOutput();
    //    if (out.m_MsgSz) {
    //        write(STDOUT_FILENO, out.m_Msg, out.m_MsgSz);
    //    }
    //}

    int  pid_status     = 0;
    bool close_frontend = false;
    if (waitpid(gdb_process, &pid_status, WNOHANG) == gdb_process) {
        int wout = write(STDOUT_FILENO, "Gdb exitted.", 11);
        UNUSED_VAR(wout);

        close_frontend = true;
    }

    // open app window and associated vulkan context
    AppWindowData app_win = { .m_CloseWin = false };
    AppLoadWindow(&app_win);
    VkStateBin* vk_ptr = LoadVulkanState(&app_win);
    SetupGuiContext(vk_ptr, &app_win);

    LoadSettings settings = { .m_MaxFileSz = (0x1 << 20) * 5 }; // 5 megabytes
    InitFrontend(&settings);

    double frame_time = NanoToSec(GetHighResTime());
    double fps        = 1.0 / 75.0;
    while (close_frontend == false && AppMustExit() == false) {
        // write(STDOUT_FILENO, "\nGdb input: ", 12);
        // char input[128];
        // fgets(input, sizeof(input), stdin);
        // if (SendCommand("%s", input)) {
        // 	GdbMsg out = GdbOutput();
        // 	if (out.m_MsgSz) {
        // 		write(STDOUT_FILENO, out.m_Msg, out.m_MsgSz);
        // 	}
        // }

        AppProcessWindowEvents(&app_win);
        close_frontend = app_win.m_CloseWin;

        ProcessGuiFrame(&app_win, DrawFrontend);

        if (waitpid(gdb_process, &pid_status, WNOHANG) == gdb_process) {
            int wout = write(STDOUT_FILENO, "Gdb exitted.", 11);
            UNUSED_VAR(wout);
            close_frontend = true;
        }

        // dont kill the machine
        double slop_time = NanoToSec(GetHighResTime()) - frame_time;
        if (slop_time < fps) {
            TimedWait(fps - slop_time);
        }
        frame_time = NanoToSec(GetHighResTime());
    }
    ShutdownGui(&app_win, CloseFrontend);

    close(fd_frontend_to_gdb[1]);

    // close gdb if still open
    if (waitpid(gdb_process, &pid_status, WNOHANG) != gdb_process) {
        kill(gdb_process, SIGKILL);

        int wout = write(STDOUT_FILENO, "\nKilling gdb and app.\n", 23);
        UNUSED_VAR(wout);
    } else {
        int wout = write(STDOUT_FILENO, "\nDone. \n", 9);
        UNUSED_VAR(wout);
    }

    return 0;
}
