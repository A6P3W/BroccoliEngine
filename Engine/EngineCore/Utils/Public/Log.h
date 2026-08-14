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

enum class ELogLevel : uint8_t { Debug = 0, Log, Warning, Error };

struct FLogEntry {
  uint64_t Sequence = 0;
  std::chrono::system_clock::time_point Timestamp;
  ELogLevel Level = ELogLevel::Log;
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
  uint64_t DroppedEntries = 0;
  bool bHistoryLost = false;
  bool bHasMore = false;
};

class BROCCOLI_ENGINE_API MLog {
 public:
  template <typename... Args>
  static void Log(
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

  static void Initialize();
  static void Shutdown();
  static void EndFrame();
  static void Flush();

  static FLogQueryResult GetRecentEntries(const FLogQuery& Query);
  static std::string_view ToLevelString(ELogLevel Level);
  static std::string FormatTimestamp(const std::chrono::system_clock::time_point& Timestamp);

 private:
  static void Write(ELogLevel Level, std::string_view Category, std::string_view Message) noexcept;
};

#define M_LOG(Level, Format, ...) MLog::Log(ELogLevel::Level, __FUNCTION__, Format, ##__VA_ARGS__)

#include "DebugOverlay.h"
