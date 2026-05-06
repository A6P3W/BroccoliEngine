#pragma once
#include <Windows.h>
#include <string>
#include <format>

class MLog {
public:
    template<typename... Args>
    static void Log(const std::string_view fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        message += "\n";
        OutputDebugStringA(message.c_str());
    }
};

#define M_LOG(fmt, ...) MLog::Log(fmt, __VA_ARGS__)
