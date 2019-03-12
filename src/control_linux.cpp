#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <ctime>

#include "config.h"

namespace fs = boost::filesystem;

#define PID_FILE "/var/run/gameap-daemon.pid"
#define LOG_DIRECTORY "/var/log/gameap-daemon/"
#define LOG_MAIN_FILE "main.log"
#define LOG_ERROR_FILE "error.log"

#define CHILD_NEED_WORK			1
#define CHILD_NEED_TERMINATE	2

int run_daemon();

// ---------------------------------------------------------------------

static void signal_error(int sig, siginfo_t *si, void *ptr)
{
	void*  ErrorAddr;
	void*  Trace[16];
	int    x;
	int    TraceSize;
	char** Messages;

    std::cout << "GameAP Daemon: " << strsignal(sig) << std::endl;

	#if __WORDSIZE == 64
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
	#else
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
	#endif

	TraceSize = backtrace(Trace, 16);
	Trace[1] = ErrorAddr;

	Messages = backtrace_symbols(Trace, TraceSize);

	if (Messages) {
		std::cerr << std::endl << "== Backtrace ==" << std::endl;

		for (x = 1; x < TraceSize; x++) {
            std::cerr << Messages[x] << std::endl;
		}

        std::cerr << "== End Backtrace ==" << std::endl << std::endl;
		free(Messages);
	}

    std::cout << "GameAP Daemon Stopped" << std::endl;
	exit(CHILD_NEED_WORK);
}

// ---------------------------------------------------------------------

void set_pid_file(const char* filename)
{
    FILE* f;

    f = fopen(filename, "w+");
    if (f) {
        fprintf(f, "%u", getpid());
        fclose(f);
    }

    fs::permissions(filename, fs::owner_write|fs::owner_read);
}

// ---------------------------------------------------------------------

int monitor_daemon()
{
    int      pid = -1;
    int      status;
    int      need_start = 1;
    sigset_t sigset;
    siginfo_t siginfo;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGQUIT);
    // sigaddset(&sigset, SIGINT);
    // sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGCHLD);
    sigaddset(&sigset, SIGUSR1);

    sigprocmask(SIG_BLOCK, &sigset, nullptr);

    set_pid_file(PID_FILE);

    for (;;)
    {
        if (need_start) {
            pid = fork();
        }

        need_start = 1;

        if (pid == -1) {
            std::cout << "GameAP Daemon Fork failed" << std::endl;
        }
        else if (!pid)
        {
            struct sigaction sigact;
            sigact.sa_flags = SA_SIGINFO;
            sigact.sa_sigaction = signal_error;
            sigemptyset(&sigact.sa_mask);

            sigaction(SIGFPE, &sigact, nullptr);
            sigaction(SIGILL, &sigact, nullptr);
            sigaction(SIGSEGV, &sigact, nullptr);
            sigaction(SIGBUS, &sigact, nullptr);

            sigemptyset(&sigset);

            sigaddset(&sigset, SIGQUIT);
            // sigaddset(&sigset, SIGINT);
            // sigaddset(&sigset, SIGTERM);

            sigaddset(&sigset, SIGUSR1);
            sigprocmask(SIG_BLOCK, &sigset, nullptr);

            status = run_daemon();
            exit(status);
        }
        else
        {
            sigwaitinfo(&sigset, &siginfo);

            if (siginfo.si_signo == SIGCHLD) {
                wait(&status);

                status = WEXITSTATUS(status);

                if (status == CHILD_NEED_TERMINATE) {
                    std::cout << "GameAP Daemon Stopped" << std::endl;
                    break;
                }
                else if (status == CHILD_NEED_WORK) {
                    std::cout << "Restarting GameAP Daemon" << std::endl;
                }
            }
            else if (siginfo.si_signo == SIGUSR1) {
                kill(pid, SIGUSR1);
                need_start = 0;
            }
            else {
                std::cout << "GameAP Daemon Monitor signal: " << strsignal(siginfo.si_signo) << std::endl;

                kill(pid, SIGTERM);
                status = 0;
                break;
            }
        }

        sleep(5);
    }

    std::cout << "GameAP Daemon stopped" << std::endl;

    unlink(PID_FILE);
    return status;
}

// ---------------------------------------------------------------------

int main(int argc, char** argv)
{
    int status;
    int pid;

	Config& config = Config::getInstance();
	config.cfg_file = "daemon.cfg";
	config.output_log = std::string(LOG_DIRECTORY) + std::string(LOG_MAIN_FILE);
	config.error_log = std::string(LOG_DIRECTORY) + std::string(LOG_ERROR_FILE);

    if (!fs::exists(LOG_DIRECTORY)) {
        fs::create_directory(LOG_DIRECTORY);
    }

    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    char buffer_time[256];
    strftime(buffer_time, sizeof(buffer_time), "%Y%m%d_%H%M", ltm);

    if (fs::exists(config.output_log) && fs::file_size(config.output_log) > 0) {
        fs::rename(
            config.output_log,
            boost::str(boost::format("%1%main_%2%.log") % LOG_DIRECTORY % buffer_time)
        );
    }

    if (fs::exists(config.error_log) && fs::file_size(config.output_log) > 0) {
        fs::rename(
            config.error_log,
            boost::str(boost::format("%1%error_%2%.log") % LOG_DIRECTORY % buffer_time)
        );
    }

	for (int i = 0; i < argc - 1; i++) {
		if (std::string(argv[i]) == "-c") {
			// Config file
			config.cfg_file = std::string(argv[i + 1]);
            i++;
        }
	}

    #ifdef NON_DAEMON
        run_daemon();
        return 0;
    #endif

	#ifdef SYSCTL_DAEMON
		monitor_daemon();
	    return 0;
	#else
	    pid = fork();

	    if (pid == -1) {
	        std::cerr << "Error: Start Daemon failed" << std::endl;
	        return -1;
	    }
	    else if (!pid) {
	        umask(0);
	        setsid();

	        fs::path exe_path( fs::initial_path<fs::path>() );
	        exe_path = fs::system_complete( fs::path( argv[0] ) );
	        fs::current_path(exe_path.parent_path());

	        close(STDIN_FILENO);
	        close(STDOUT_FILENO);
	        close(STDERR_FILENO);

	        freopen(config.output_log.c_str(), "w", stdout);
	        freopen(config.error_log.c_str(), "w", stderr);

	        // run_daemon();
	        monitor_daemon();

	        status = 0;

	        return status;
	    }
	    else {
	        return 0;
	    }
	#endif
}
