#include <iostream>

#include "include/memory.h"
#include "include/check.h"

using namespace Nexus::Base;
using namespace Nexus::Check;


struct test {
    int k;
    float a;
    char w[3];
};

mayfail<char[3]> wq(bool i) {
    if (i) {
        char arr[3] = {1, 2, 3};
        return arr;
    } else {
        return failed;
    }
}

int main() {
    UniquePool up(1024);
    Stream<UniquePool<HeapAllocator>> stream(std::move(up));
    stream.next(test{12, 1234.f, {1, 2, 3}});
    stream.rewind();
    auto k = stream.next<int>();
    auto a = stream.next<float>();
    auto w = stream.next<char[3]>();
}
