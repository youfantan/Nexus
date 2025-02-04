#include "include/mem/memory.h"
#include "include/io/socket.h"
#include "include/utils/unexpected.h"
#include "include/platform/win32/win32_net.h"
#include "include/utils/netaddr.h"
#include <thread>
using namespace Nexus::Base;
using namespace Nexus::Utils;
using namespace Nexus::Net;

int main() {
    UniquePool<> up(64);
    WSAData wsadat;
    if (WSAStartup(MAKEWORD(2,2), &wsadat) != 0) {
        BREAKPOINTM("Error at initialize WSA");
    }
    Socket server(SockType::SOCK_IPV4);
    server.bind("127.0.0.1", 2241);
    std::thread trd([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds (1000));
        Socket client(SockType::SOCK_IPV4);
        client.connect("127.0.0.1", 2241);
    });
    server.listen();
    Socket cli = server.accept();
    trd.join();
}
