#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "BroccoliEngineAPI.h"

enum class ELogLevel : uint8_t { Debug = 0, Info, Warning, Error };

struct FLogEntry {
  uint64_t Sequence = 0;
  std::chrono::system_clock::time_point Timestamp;
  ELogLevel Level = ELogLevel::Info;
  std::string Category;
  std::string Message;
};

struct FLogQuery {
  size_t Limit = 100;
  std::optional<ELogLevel> MinimumLevel;
  std::optional<uint64_t> AfterSequence;
};

struct FLogQueryResult {
  std::vector<FLogEntry> Entries;
  uint64_t OldestAvailableSequence = 0;
  uint64_t LatestSequence = 0;
  uint64_t NextAfterSequence = 0;
  bool bHistoryLost = false;
  bool bHasMore = false;
};

class BROCCOLI_ENGINE_API MLog {
 public:
  template <typename... Args>
  static void Log(const char* FunctionName, std::string_view Format, Args&&... Arguments) noexcept {
    LogWithLevel(ELogLevel::Info, FunctionName, Format, std::forward<Args>(Arguments)...);
  }

  template <typename... Args>
  static void LogWithLevel(
      ELogLevel Level, const char* FunctionName, std::string_view Format, Args&&... Arguments
  ) noexcept {
    try {
      Write(
          Level,
          FunctionName ? std::string_view(FunctionName) : std::string_view(),
          std::vformat(Format, std::make_format_args(Arguments...))
      );
    } catch (...) {
      Write(
          ELogLevel::Error,
          FunctionName ? std::string_view(FunctionName) : std::string_view("MLog"),
          "Log message formatting failed."
      );
    }
  }

  static FLogQueryResult GetRecentEntries(const FLogQuery& Query);
  static std::string_view ToLevelString(ELogLevel Level);
  static std::string FormatTimestamp(const std::chrono::system_clock::time_point& Timestamp);

 private:
  static void Write(ELogLevel Level, std::string_view Category, std::string_view Message) noexcept;
};

#define M_LOG(fmt, ...) MLog::LogWithLevel(ELogLevel::Info, __FUNCTION__, fmt, ##__VA_ARGS__)
#define M_LOG_DEBUG(fmt, ...) MLog::LogWithLevel(ELogLevel::Debug, __FUNCTION__, fmt, ##__VA_ARGS__)
#define M_LOG_WARNING(fmt, ...) \
  MLog::LogWithLevel(ELogLevel::Warning, __FUNCTION__, fmt, ##__VA_ARGS__)
#define M_LOG_ERROR(fmt, ...) MLog::LogWithLevel(ELogLevel::Error, __FUNCTION__, fmt, ##__VA_ARGS__)

#include "DebugOverlay.h"
