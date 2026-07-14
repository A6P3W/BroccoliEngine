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

      OutputDebugStringA(message.c_str());
      std::cout << message;

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
