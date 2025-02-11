#pragma once

#include <string>
#include <format>
#include <iostream>
#include <fstream>
#include "../utils/unexpected.h"

namespace Nexus::Log {
    enum class log_level {
        TRACE,
        INFO,
        WARNING,
        ERR,
        FATAL
    };

    inline std::ofstream lf;

    inline std::string format_time(const std::string& fmt) {
        auto now = std::chrono::system_clock::now(); // 获取当前时间
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&time_t_now);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, fmt.c_str()); // 格式化时间
        return oss.str();
    }

    inline static void log_init() {
        std::filesystem::path lgf(std::format("{}{}.log", "./log/", format_time("%Y-%m-%d_%H_%M_%S")));
        lf.open(lgf, std::ios::out | std::ios::binary);
        if (!lf.is_open()) {
            std::cout << "Cannot create log file:" << lgf << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline static void log_stop() {
        lf.close();
    }

    template<log_level lv, typename... Args>
    inline static void logout(std::format_string<Args...> fmt, const char* file, int line, Args&&... args) {
        std::string lgmsg = std::format(fmt, std::forward<Args>(args)...);
        std::string level;
        std::string ansi_suffix = "\x1b[37m";
        std::string ansi_prefix;
        switch (lv) {
            case log_level::INFO: level = "INFO"; ansi_prefix = "\x1b[36m"; break;
            case log_level::TRACE: level = "TRACE"; ansi_prefix = "\x1b[37m"; break;
            case log_level::WARNING: level = "WARNING"; ansi_prefix = "\x1b[33m"; break;
            case log_level::ERR: level = "ERROR"; ansi_prefix = "\x1b[31m"; break;
            case log_level::FATAL: level = "FATAL"; ansi_prefix = "\x1b[31m"; break;
        }
        std::string msg = std::format("[{}]{}({}:{})", level, format_time("[%Y-%m-%d][%H:%M:%S]"), file, line).append(lgmsg);
#ifdef LOG_ANSI_SUPPORT
        std::string consolemsg = ansi_prefix.append(msg).append(ansi_suffix);
#else
        std::string consolemsg = msg;
#endif
        std::cout << consolemsg << std::endl;
        lf.write(msg.c_str(), static_cast<int>(msg.size()));
        lf.write("\r\n", 2);
        lf.flush();
    }
}

#define LINFO(msg, ...) Nexus::Log::logout<Nexus::Log::log_level::INFO>(msg, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define LTRACE(msg, ...) Nexus::Log::logout<Nexus::Log::log_level::TRACE>(msg, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define LERROR(msg, ...) Nexus::Log::logout<Nexus::Log::log_level::ERR>(msg, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define LWARN(msg, ...) Nexus::Log::logout<Nexus::Log::log_level::WARNING>(msg, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define LFATAL(msg, ...) Nexus::Log::logout<Nexus::Log::log_level::FATAL>(msg, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
