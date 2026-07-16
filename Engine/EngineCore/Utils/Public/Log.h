#pragma once
#include <Windows.h>

#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

class MLog {
 private:
  static std::wstring Utf8ToWide(const std::string& Value) {
    if (Value.empty()) {
      return {};
    }

    const int Length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, Value.data(), static_cast<int>(Value.size()), nullptr, 0
    );
    if (Length <= 0) {
      return {};
    }

    std::wstring Result(static_cast<size_t>(Length), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Value.data(),
        static_cast<int>(Value.size()),
        Result.data(),
        Length
    );
    return Result;
  }

  static inline std::filesystem::path log_filepath = []() {
    std::filesystem::create_directories("Logs");

    std::time_t T = std::time(nullptr);
    std::tm LocalTime;
    localtime_s(&LocalTime, &T);
    char Buf[64];
    std::strftime(Buf, sizeof(Buf), "%Y-%m-%d_%H-%M-%S", &LocalTime);
    std::string Filename = std::string(Buf) + ".log";

    return std::filesystem::path("Logs") / Filename;
  }();

 public:
  template <typename... Args>
  static void Log(const char* func_name, const std::string_view fmt, Args&&... args) {
    try {
      std::string formatted_user_msg = std::vformat(fmt, std::make_format_args(args...));
      std::string message = std::format("[{}] {}\n", func_name, formatted_user_msg);

      const std::wstring WideMessage = Utf8ToWide(message);
      if (!WideMessage.empty()) {
        OutputDebugStringW(WideMessage.c_str());

        const HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD ConsoleMode = 0;
        if (Console != INVALID_HANDLE_VALUE && GetConsoleMode(Console, &ConsoleMode) != FALSE) {
          DWORD Written = 0;
          WriteConsoleW(
              Console, WideMessage.data(), static_cast<DWORD>(WideMessage.size()), &Written, nullptr
          );
        } else {
          std::cout << message;
        }
      } else {
        OutputDebugStringA(message.c_str());
        std::cout << message;
      }

      std::ofstream log_file(log_filepath, std::ios::app);
      if (log_file.is_open()) {
        log_file << message;
      }
    } catch (const std::exception& e) {
      std::cerr << "[MLog Error] " << e.what() << std::endl;
    }
  }
};

#define M_LOG(fmt, ...) MLog::Log(__FUNCTION__, fmt, ##__VA_ARGS__)

#include "DebugOverlay.h"
