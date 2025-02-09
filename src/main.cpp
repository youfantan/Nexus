#include "include/mem/memory.h"
#include "include/net/socket.h"
#include "include/log/logger.h"
#include "include/net/http_server.h"
#include "include/net/basic_handlers.h"
#include "include/net/https_server.h"
#include "include/io/terminal.h"
#include "include/platform/win32/win32_io.h"
#include <thread>

using namespace Nexus::Base;
using namespace Nexus::Utils;
using namespace Nexus::Net;

int main() {
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
    WorkGroup<CPU_CORES> group;
    HttpsServer<Nexus::IO::Win32SelectMUX, CPU_CORES> https(NetAddr("0.0.0.0", 443), group);
    HttpServer <Nexus::IO::Win32SelectMUX, CPU_CORES> http(NetAddr("0.0.0.0", 80), group);
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
