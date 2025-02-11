#include "include/mem/memory.h"
#include "include/net/socket.h"
#include "include/log/logger.h"
#include "include/net/http_server.h"
#include "include/net/basic_handlers.h"
#include "include/net/https_server.h"
#include "include/io/terminal.h"
#include "include/platform/win32/win32_io.h"
#include <thread>
#include <dbghelp.h>

using namespace Nexus::Base;
using namespace Nexus::Utils;
using namespace Nexus::Net;

void CreateDump(EXCEPTION_POINTERS* pExceptionInfo) {
    HANDLE hFile = CreateFile("crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = pExceptionInfo;
        dumpInfo.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &dumpInfo, nullptr, nullptr);
        CloseHandle(hFile);
    }
}


LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    FILE* logFile = fopen("crash.log", "a");
    if (logFile) {
        fprintf(logFile, "Crash detected! Exception code: 0x%lX\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
        fprintf(logFile, "Fault address: 0x%p\n", pExceptionInfo->ExceptionRecord->ExceptionAddress);
        fclose(logFile);
    }
    CreateDump(pExceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
    SetUnhandledExceptionFilter(ExceptionHandler);
    using namespace Nexus::Net;
    using namespace Nexus::Utils;
    using namespace Nexus::Parallel;
#ifdef PLATFORM_WIN32
    Nexus::IO::EnableWindowsVirtualANSI();
#endif
    Nexus::Log::log_init();
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        LFATAL("WSAStartup Failed. Error Code: {}", err);
        return 0;
    }
    WorkGroup<CPU_CORES - 1> group;
    HttpsServer<Nexus::IO::Win32SelectMUX, CPU_CORES - 1> https(NetAddr("0.0.0.0", 443), group);
    HttpServer <Nexus::IO::Win32SelectMUX, CPU_CORES - 1> http(NetAddr("0.0.0.0", 80), group);
    https.add_handler<statistics_handler>("/statistics");
    http.add_handler<statistics_handler>("/statistics");
    while (true) {
        https.loop();
        http.loop();
        if (Nexus::IO::getch() == 0) {
            LINFO("Quit key pressed. Now Exiting...");
            break;
        }
    }
    group.cleanup();
    https.close();
    http.close();
    Nexus::Log::log_stop();
    return 0;
}
