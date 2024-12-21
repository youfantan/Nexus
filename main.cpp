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
    SharedPool up(4);
    for (int i = 1; i < 5; ++i) {
        std::thread tr ([&up, i]() {
            auto stream = Stream(up);
            stream.position((i - 1) * 4096);
            for (int j = 0; j < 1024; ++j) {
                bool b = stream.next(i);
            }
            std::cout << "finished" << std::endl;
        });
        tr.join();
    }
    uint64_t accmu = 0;
    auto stream = Stream(up);
    stream.rewind();
    auto wq = stream.next<int>();
    for (int i = 0; i < 4096; ++i) {
        accmu += wq.result();
        wq = stream.next<int>();
    }
    auto dwe = stream.next<int>();
}
