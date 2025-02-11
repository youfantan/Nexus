#include <include/net/basic_handlers.h>
#include <include/net/https_server.h>
#include <include/net/http_server.h>

http_response statistics_handler::doGet(const get_request &gr) {
    Nexus::Base::UniquePool<> resp(1024);
    auto data = std::to_string(Nexus::Net::executed_sock + Nexus::Net::executed_tls);
    resp.write(data.c_str(), data.size());
    return {"200 OK",{
            {"Content-Type", "text/plain"}
    }, Nexus::Base::unique_to_readonly<Nexus::Base::HeapAllocator>(std::move(resp))};
}

http_response statistics_handler::doPost(const post_request &pr) {
    return {"405 Method Not Allowed",{}, Nexus::Base::FixedPool<true>(nullptr, 0)};
}
