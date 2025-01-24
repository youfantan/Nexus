#pragma once

#include <string>

#ifdef PLATFORM_WIN32
#include "../platform/win32/win32_stacktrace.h"
#endif

namespace Nexus::Utils {
#define BREAKPOINT breakpoint(__FILE__, __LINE__, "")
#define BREAKPOINTM(msg) breakpoint(__FILE__, __LINE__, msg)
    constexpr std::string find_relative_path(const std::string& path) {
        std::string current_path = __FILE__;
        return path.substr(current_path.size() - 33);
    }
    void breakpoint(const std::string& path, int line, const std::string& msg) {
        if (msg.empty()) {
            std::cout << "Triggered BreakPoint: " << find_relative_path(path)<< ":" << line << std::endl;
        } else {
            std::cout << "Triggered BreakPoint: " << find_relative_path(path)<< ":" << line << " | " << msg << std::endl;
        }
    }
    void stacktrace() {
    }
}