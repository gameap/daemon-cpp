#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstdlib>

#include <cstring>

#include <execinfo.h>
#include <unistd.h>
#include <wait.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <plog/Appenders/ColorConsoleAppender.h>

#include "log.h"
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

    GAMEAP_LOG_ERROR << "GameAP Daemon: " << strsignal(sig);

	#if __WORDSIZE == 64
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
	#else
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
	#endif

	TraceSize = backtrace(Trace, 16);
	Trace[1] = ErrorAddr;

	Messages = backtrace_symbols(Trace, TraceSize);

	if (Messages) {
        GAMEAP_LOG_ERROR << std::endl << "== Backtrace ==";

		for (x = 1; x < TraceSize; x++) {
            GAMEAP_LOG_ERROR << Messages[x];
		}

        GAMEAP_LOG_ERROR << std::endl << "== End Backtrace ==" << std::endl;
		free(Messages);
	}

    GAMEAP_LOG_INFO << "GameAP Daemon Stopped";
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

void monitor_daemon()
{
    int      pid = -1;
    int      need_start = 1;
    sigset_t sigset;
    siginfo_t siginfo;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
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
            GAMEAP_LOG_FATAL << "GameAP Daemon Fork failed";
        }
        else if (!pid) {
            struct sigaction sigact{};
            sigact.sa_flags = SA_SIGINFO;
            sigact.sa_sigaction = signal_error;
            sigemptyset(&sigact.sa_mask);

            sigaction(SIGFPE, &sigact, nullptr);
            sigaction(SIGILL, &sigact, nullptr);
            sigaction(SIGSEGV, &sigact, nullptr);
            sigaction(SIGBUS, &sigact, nullptr);

            sigemptyset(&sigset);

            sigaddset(&sigset, SIGHUP);
            sigaddset(&sigset, SIGQUIT);
            sigaddset(&sigset, SIGINT);
            sigaddset(&sigset, SIGTERM);
            sigaddset(&sigset, SIGUSR1);

            sigprocmask(SIG_UNBLOCK, &sigset, nullptr);

            int run_daemon_status = run_daemon();
            exit(run_daemon_status);
        }
        else {
            sigwaitinfo(&sigset, &siginfo);

            if (siginfo.si_signo == SIGCHLD) {
                int status;
                wait(&status);

                status = WEXITSTATUS(status);

                if (status == CHILD_NEED_TERMINATE) {
                    GAMEAP_LOG_INFO << "GameAP Daemon Stopped";
                    break;
                }
                else if (status == CHILD_NEED_WORK) {
                    GAMEAP_LOG_INFO << "Restarting GameAP Daemon";
                }
            }
            else if (siginfo.si_signo == SIGUSR1) {
                kill(pid, SIGUSR1);
                need_start = 0;
            }
            else if (siginfo.si_signo == SIGHUP) {
                kill(pid, SIGHUP);
                need_start = 0;
            }
            else {
                GAMEAP_LOG_INFO << "GameAP Daemon Monitor signal: " << strsignal(siginfo.si_signo);

                kill(pid, siginfo.si_signo);
                int status = 0;

                // Waiting childs
                wait(&status);

                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    GAMEAP_LOG_INFO << "GameAP Daemon stopped";

    unlink(PID_FILE);
}

// ---------------------------------------------------------------------

int main(int argc, char** argv)
{
	Config& config = Config::getInstance();
	config.cfg_file = "daemon.cfg";

    fs::path exe_path( fs::initial_path<fs::path>() );
    exe_path = fs::system_complete( fs::path( argv[0] ) );

    std::string path_env = getenv("PATH");
    path_env += ":" + exe_path.parent_path().string();
    std::string put_env = "PATH=" + path_env;
    putenv(&put_env[0]);

    #ifdef CONSOLE_LOG
        static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
        plog::init<GameAP::MainLog>(plog::verbose, &consoleAppender);
        plog::init<GameAP::ErrorLog>(plog::verbose, &consoleAppender);
    #else

        std::string log_directory = (getuid() == 0) ? std::string(LOG_DIRECTORY) : "logs/";

        config.output_log = log_directory + std::string(LOG_MAIN_FILE);
        config.error_log = log_directory + std::string(LOG_ERROR_FILE);

        if (!fs::exists(log_directory)) {
            fs::create_directory(log_directory);
        }

        time_t now = time(nullptr);
        tm *ltm = localtime(&now);
        char buffer_time[256];
        strftime(buffer_time, sizeof(buffer_time), "%Y%m%d_%H%M", ltm);

        if (fs::exists(config.output_log) && fs::file_size(config.output_log) > 0) {
            fs::rename(
                config.output_log,
                boost::str(boost::format("%1%main_%2%.log") % log_directory % buffer_time)
            );
        }

        if (fs::exists(config.error_log) && fs::file_size(config.error_log) > 0) {
            fs::rename(
                config.error_log,
                boost::str(boost::format("%1%error_%2%.log") % log_directory % buffer_time)
            );
        }

        for (int i = 0; i < argc - 1; i++) {
            if (std::string(argv[i]) == "-c") {
                // Config file
                config.cfg_file = std::string(argv[i + 1]);
                i++;
            }
        }

        plog::init<GameAP::MainLog>(plog::verbose, config.output_log.c_str());
        plog::init<GameAP::ErrorLog>(plog::verbose, config.error_log.c_str());
    #endif

	#ifdef SYSCTL_DAEMON
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        freopen(config.output_log.c_str(), "w", stdout);
        freopen(config.error_log.c_str(), "w", stderr);

		monitor_daemon();
	    return 0;
    #elif NON_DAEMON
        run_daemon();
        return 0;
	#else
	    int pid = fork();

	    if (pid == -1) {
	        GAMEAP_LOG_FATAL << "Error: Start Daemon failed";
	        return -1;
	    }
	    else if (!pid) {
	        umask(0);
	        setsid();

	        fs::current_path(exe_path.parent_path());

	        close(STDIN_FILENO);
	        close(STDOUT_FILENO);
	        close(STDERR_FILENO);

	        freopen(config.output_log.c_str(), "w", stdout);
	        freopen(config.error_log.c_str(), "w", stderr);

	        // run_daemon();
	        monitor_daemon();

	        return 0;
	    }
	    else {
	        return 0;
	    }
	#endif
}
