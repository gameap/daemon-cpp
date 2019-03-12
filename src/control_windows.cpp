#include <boost/filesystem.hpp>

#include <Windows.h>
#include <tchar.h>

#include <boost/format.hpp>

#include "config.h"

#define LOG_DIRECTORY "log/"
#define LOG_MAIN_FILE "main.log"
#define LOG_ERROR_FILE "error.log"

namespace fs = boost::filesystem;

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

int run_daemon();

#define SERVICE_NAME  _T("GameAP Daemon")

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;

	OutputDebugString(_T("GDaemon: ServiceMain: Entry"));

	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		OutputDebugString(_T("GDaemon: ServiceMain: RegisterServiceCtrlHandler returned error"));
		return;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof (g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T("GDaemon: ServiceMain: SetServiceStatus returned error"));
	}

	/*
	* Perform tasks neccesary to start the service here
	*/
	OutputDebugString(_T("GDaemon: ServiceMain: Performing Service Start Operations"));

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		OutputDebugString(_T("GDaemon: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			OutputDebugString(_T("GDaemon: ServiceMain: SetServiceStatus returned error"));
		}
		
		return;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T("GDaemon: ServiceMain: SetServiceStatus returned error"));
	}

	// Start the thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	OutputDebugString(_T("GDaemon: ServiceMain: Waiting for Worker Thread to complete"));

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	OutputDebugString(_T("GDaemon: ServiceMain: Worker Thread Stop Event signaled"));


	/*
	* Perform any cleanup tasks
	*/
	OutputDebugString(_T("GDaemon: ServiceMain: Performing Cleanup Operations"));

	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		OutputDebugString(_T("GDaemon: SetServiceStatus returned error"));
	}

	return;
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	OutputDebugString(_T("GDaemon: ServiceCtrlHandler: Entry"));

	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		OutputDebugString(_T("GDaemon: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks neccesary to stop the service here
		*/
		g_ServiceStatus.dwControlsAccepted = 0;
		//g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;


		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			OutputDebugString(_T("GDaemon: ServiceCtrlHandler: SetServiceStatus returned error"));
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);
		//stop_daemon();

		// exit(ERROR_SUCCESS);

		break;

	default:
		break;
	}

	OutputDebugString(_T("GDaemon: ServiceCtrlHandler: Exit"));
}

int _tmain (int argc, TCHAR *argv[])
{
    fs::path exe_path( fs::initial_path<fs::path>() );
    exe_path = fs::system_complete( fs::path(argv[0]) );
    fs::current_path(exe_path.parent_path());

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

    freopen(config.output_log.c_str(), "w", stdout);
    freopen(config.error_log.c_str(), "w", stderr);

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher (ServiceTable) == FALSE)
    {
        return GetLastError ();
    }
}

/*
int main(int argc, char** argv)
{
	fs::path exe_path( fs::initial_path<fs::path>() );
    exe_path = fs::system_complete( fs::path( argv[0] ) );
    fs::current_path(exe_path.parent_path());

    ServiceWorkerThread();
	
}
*/

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	OutputDebugString(_T("GDaemon: ServiceWorkerThread: Entry"));

	bool started = false;

	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(1000);
		if (!started) {
			run_daemon();
			started = true;
		}
	}

	OutputDebugString(_T("GDaemon: ServiceWorkerThread: Exit"));
	return ERROR_SUCCESS;
}
