#include "include/mem/memory.h"
#include "include/net/socket.h"
#include "include/utils/unexpected.h"
#include "include/platform/win32/win32_net.h"
#include "include/utils/netaddr.h"
#include "include/net/http_server.h"
#include <thread>
using namespace Nexus::Base;
using namespace Nexus::Utils;
using namespace Nexus::Net;

class TestHttpHandler {
public:
    TestHttpHandler() = default;
    ~TestHttpHandler() = default;
    static http_response doGet(const get_request& gr) {
        Nexus::Base::SharedPool<> resp(1024);
        std::string str = "<html><body>Hello World</body></html>";
        resp.write(str.data(), str.size());
        return {"200 OK",{
                {"Content-Type", "text/html"}
        }, resp};
    }
    static http_response doPost(const post_request& gr) {
        Nexus::Base::SharedPool<> resp(1024);
        std::string str = "Test Data";
        resp.write(str.data(), str.size());
        return {"200 OK",{
                {"Content-Type", "text/plain"}
        }, resp};
    }
};

int main() {
    using namespace Nexus::Net;
    using namespace Nexus::Utils;
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 0;
    }
    HttpServer server(NetAddr("0.0.0.0", 80));
    server.add_handler<TestHttpHandler>("/test");
    server.listen();
    return 0;
}
