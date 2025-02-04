#pragma once

#include <vector>
#include <iostream>
using test_task = bool(*)();

static std::vector<test_task> tasks;

#define test_assert(expr) if ((expr) == false) return false
#define mayfail_assert(expr) if (!(expr).is_valid()) return false
inline static void RegisterTask(test_task tsk) {
    tasks.push_back(tsk);
}

inline static void ExecuteAll() {
    int n = 1;
    for (auto& tsk : tasks) {
        std::cout << std::format("Executing Test Task: {}/{}", n, tasks.size()) << std::endl;
        bool r = tsk();
        if (!r) {
            std::cout << std::format("Task {} Failed", n) << std::endl;
            return;
        }
        std::cout << std::format("Task {} Success", n) << std::endl;
        ++n;
    }
}

inline static bool PtrCompare(void* a, void* b, size_t sz) {
    for (int i = 0; i < sz; ++i) {
        if (reinterpret_cast<char*>(a)[i] != reinterpret_cast<char*>(b)[i]) return false;
    }
    return true;
}