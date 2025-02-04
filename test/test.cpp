#include "../include/mem/memory.h"
#include "test_framework.h"
#include "unit_memory.cpp"
int main() {
    using namespace Nexus::Test::Base;
    RegisterTask(UniquePoolTest);
    RegisterTask(UniqueFlexHolderTest);
    ExecuteAll();
}