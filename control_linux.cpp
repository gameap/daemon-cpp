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

// TODO: Debug only. Delete it
#include <openssl/bio.h>

#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include "config.h"

#include "functions/gcrypt.h"

#define PID_FILE "/var/run/gdaemon.pid"
// #define PID_FILE "gdaemon.pid"

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
		std::cout << std::endl << "== Backtrace ==" << std::endl;

		for (x = 1; x < TraceSize; x++) {
            std::cout << Messages[x] << std::endl;
		}

        std::cout << "== End Backtrace ==" << std::endl << std::endl;
		free(Messages);
	}

    std::cout << "GameAP Daemon Stopped" << std::endl;
	exit(CHILD_NEED_WORK);
}

// ---------------------------------------------------------------------

void set_pid_file(char* filename)
{
    FILE* f;

    f = fopen(filename, "w+");
    if (f) {
        fprintf(f, "%u", getpid());
        fclose(f);
    }
}

// ---------------------------------------------------------------------

int monitor_daemon()
{
    int      pid;
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

    sigprocmask(SIG_BLOCK, &sigset, NULL);

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

            sigaction(SIGFPE, &sigact, 0);
            sigaction(SIGILL, &sigact, 0);
            sigaction(SIGSEGV, &sigact, 0);
            sigaction(SIGBUS, &sigact, 0);

            sigemptyset(&sigset);

            sigaddset(&sigset, SIGQUIT);
            // sigaddset(&sigset, SIGINT);
            // sigaddset(&sigset, SIGTERM);

            sigaddset(&sigset, SIGUSR1);
            sigprocmask(SIG_BLOCK, &sigset, NULL);

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

//    char* text = "Text";
    char *enc_text;
    char *dec_text;

    char *res_test = new char;
    char *res_test2 = new char;

//    std::shared_ptr<char> res_test_sh(new char);
//    std::shared_ptr<char *> res_test2(new char);

//    char *sh = res_test_sh.get();

    size_t enc_size = GCrypt::aes_encrypt(&res_test2, "abcdcfdeabcdcfde", 16, "12345678123456781234567812345678", "1234567812345678");
    size_t dec_size = GCrypt::aes_decrypt(&res_test2, res_test, enc_size, "12345678123456781234567812345678", "1234567812345678");
    std::cout << "res_test2: " << res_test2 << std::endl;
    exit;

	Config& config = Config::getInstance();
	config.cfg_file = "daemon.cfg";
	config.error_log = "/var/log/gdaemon/error.log";

	for (int i = 0; i < argc - 1; i++) {
		if (std::string(argv[i]) == "-c") {
			// Config file
			config.cfg_file = std::string(argv[i + 1]);
            i++;
        }
	}

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

	        boost::filesystem::path exe_path( boost::filesystem::initial_path<boost::filesystem::path>() );
	        exe_path = boost::filesystem::system_complete( boost::filesystem::path( argv[0] ) );
	        boost::filesystem::current_path(exe_path.parent_path());

	        close(STDIN_FILENO);
	        close(STDOUT_FILENO);
	        close(STDERR_FILENO);

	        freopen(config.error_log.c_str(), "w", stdout);
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
