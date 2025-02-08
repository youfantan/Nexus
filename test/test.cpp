#include "test_framework.h"
#include "unit_memory.hpp"
#include "include/net/http_server.h"
#include <include/mem/memory.h>
#include <include/utils/netaddr.h>

#include <filesystem>

inline static bool HttpServerTest() {
    return true;
}
int main() {
    using namespace Nexus::Test::Base;
    RegisterTask(SharedPoolTest);
    RegisterTask(UniquePoolTest);
    RegisterTask(UniqueFlexHolderTest);
    RegisterTask(HttpServerTest);
    ExecuteAll();
}