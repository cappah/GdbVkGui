#include <inttypes.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>

#include "ProcessIO.h"

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
			(char*)g_exe,
			(char*)"--interpreter=mi",
			(char*)"--quiet",
			NULL
		};
		execvp(gdb_argv[0], gdb_argv);

		_exit(0);
	}
	return gdb_process;
}

int
main(const int argc, const char* argv[])
{
	// Make sure gdb is an exe that exists
	const char* gdb_exe = "/usr/bin/gdb";
	if (access(gdb_exe, X_OK)) {
		PrintErr("Failed to find gdb exe: ");
	}

	// Open non-blocking pipes for inter-process communication
	// [0] : read end 
	// [1] : write end
	int* fd_frontend_to_gdb = GetFtoGPipes();
	int* fd_gdb_to_frontend = GetGtoFPipes();

	if (pipe2(fd_frontend_to_gdb, O_NONBLOCK) || 
		pipe2(fd_gdb_to_frontend, O_NONBLOCK)) {
		PrintErr("Failed to open pipes for gdb communication: ");
	}

	pid_t gdb_process = CreateGdbProcess(gdb_exe, fd_frontend_to_gdb, fd_gdb_to_frontend);

	if (SendCommand("-gdb-version")) {
		GdbMsg out = GdbOutput();
		if (out.m_MsgSz) {
			write(STDOUT_FILENO, out.m_Msg, out.m_MsgSz);
		}
	}
		
	int pid_status = 0;
	bool close_frontend = false;
	if (waitpid(gdb_process, &pid_status, WNOHANG) == gdb_process) {
		write(STDOUT_FILENO, "Gdb exitted.", 11);
		close_frontend = true;
	}

	while (close_frontend == false) {
		write(STDOUT_FILENO, "\nGdb input: ", 12);
		char input[128];
		fgets(input, sizeof(input), stdin);
		if (SendCommand("%s", input)) {
			GdbMsg out = GdbOutput();
			if (out.m_MsgSz) {
				write(STDOUT_FILENO, out.m_Msg, out.m_MsgSz);
			}
		}

		if (waitpid(gdb_process, &pid_status, WNOHANG) == gdb_process) {
			write(STDOUT_FILENO, "Gdb exitted.", 11);
			close_frontend = true;
		}
	}
	close(fd_frontend_to_gdb[1]);

	write(STDOUT_FILENO, "\nDone. \n", 9);

	// close gdb if still open
	if (waitpid(gdb_process, &pid_status, WNOHANG) != gdb_process) {
		kill(gdb_process, SIGKILL);
	}

	return 0;
}
