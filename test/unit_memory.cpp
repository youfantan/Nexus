#include "test_framework.h"
#include <include/mem/memory.h>

namespace Nexus::Test::Base {
    struct data {
        int d0;
        float d1;
        char d2[24];
    };

    UniquePool<> getup() {
        UniquePool<> up(48);
        Stream<UniquePool<>> stream(std::move(up));
        stream.next(data {1, 14.51f, {4}});
        char d3[32] = {1, 1, 4, 5, 1, 4};
        stream.write(d3, 32);
        stream.next(static_cast<int>(114514));
        return std::move(stream.container());
    }

    inline static bool UniquePoolTest() {
        using namespace Nexus::Utils;
        using namespace Nexus::Base;
        auto tp = getup();
        Stream<UniquePool<>> stream(std::move(tp));
        auto d0 = stream.next<int>();
        mayfail_assert(d0);
        test_assert(d0.reference() == 1);
        auto d1 = stream.next<float>();
        mayfail_assert(d1);
        test_assert(d1.reference() == 14.51f);
        auto d2 = stream.next<decltype(data::d2)>();
        mayfail_assert(d2);
        auto d2_r = d2.reference(); // using gdb
        auto d3 = stream.read(32);
        mayfail_assert(d3);
        auto d3_r = d3.result();
        auto d4 = stream.next<int>();
        mayfail_assert(d4);
        test_assert(d4.reference() == 114514);
        stream.~Stream();
        return true;
    }

    auto getholder() {
        char data[32] = {1, 1, 4, 5, 1, 4, 1, 9, 1, 9, 8, 1, 0};
        return UniqueFlexHolder(data);
    }

    inline static bool UniqueFlexHolderTest() {
        auto holder = getholder();
        char data[32] = {1, 1, 4, 5, 1, 4, 1, 9, 1, 9, 8, 1, 0};
        test_assert(PtrCompare(data, holder.ptr(), 32));
        holder.~UniqueFlexHolder();
        return true;
    }

}