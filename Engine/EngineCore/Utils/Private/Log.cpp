#include "Log.h"

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace {
constexpr size_t MaxLogEntries = 1000;
constexpr size_t MaxLogMessageBytes = 4096;
constexpr size_t MaxLogCategoryBytes = 128;

struct FMLogState {
  std::mutex BufferMutex;
  std::deque<FLogEntry> Entries;
  uint64_t NextSequence = 1;

  std::mutex SinkMutex;
  std::filesystem::path LogFilePath;
};

FMLogState& GetLogState() {
  static FMLogState State;
  return State;
}

bool IsContinuationByte(unsigned char Byte) { return (Byte & 0xC0u) == 0x80u; }

size_t GetValidUtf8CharacterLength(std::string_view Value, size_t Offset) {
  const unsigned char Lead = static_cast<unsigned char>(Value[Offset]);
  size_t Length = 0;
  uint32_t CodePoint = 0;
  if (Lead <= 0x7Fu) {
    return 1;
  }
  if (Lead >= 0xC2u && Lead <= 0xDFu) {
    Length = 2;
    CodePoint = Lead & 0x1Fu;
  } else if (Lead >= 0xE0u && Lead <= 0xEFu) {
    Length = 3;
    CodePoint = Lead & 0x0Fu;
  } else if (Lead >= 0xF0u && Lead <= 0xF4u) {
    Length = 4;
    CodePoint = Lead & 0x07u;
  } else {
    return 0;
  }

  if (Offset + Length > Value.size()) {
    return 0;
  }
  for (size_t Index = 1; Index < Length; ++Index) {
    const unsigned char Byte = static_cast<unsigned char>(Value[Offset + Index]);
    if (!IsContinuationByte(Byte)) {
      return 0;
    }
    CodePoint = (CodePoint << 6u) | (Byte & 0x3Fu);
  }

  const bool Overlong = (Length == 2 && CodePoint < 0x80u) || (Length == 3 && CodePoint < 0x800u) ||
                        (Length == 4 && CodePoint < 0x10000u);
  if (Overlong || (CodePoint >= 0xD800u && CodePoint <= 0xDFFFu) || CodePoint > 0x10FFFFu) {
    return 0;
  }
  return Length;
}

std::string TruncateUtf8(std::string_view Value, size_t MaxBytes) {
  const bool NeedsTruncation = Value.size() > MaxBytes;
  const size_t ContentLimit = NeedsTruncation && MaxBytes >= 3 ? MaxBytes - 3 : MaxBytes;
  std::string Result;
  Result.reserve((std::min)(Value.size(), MaxBytes));

  size_t Offset = 0;
  while (Offset < Value.size()) {
    const size_t Length = GetValidUtf8CharacterLength(Value, Offset);
    const size_t BytesToAppend = Length == 0 ? 1 : Length;
    if (Result.size() + BytesToAppend > ContentLimit) {
      break;
    }
    if (Length == 0) {
      Result.push_back('?');
      ++Offset;
    } else {
      Result.append(Value.substr(Offset, Length));
      Offset += Length;
    }
  }

  if (Offset < Value.size() && MaxBytes >= 3) {
    Result.append("...");
  }
  return Result;
}

std::wstring Utf8ToWide(std::string_view Value) {
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

std::filesystem::path CreateLogFilePath() {
  std::filesystem::create_directories("Logs");

  const std::time_t Time = std::time(nullptr);
  std::tm LocalTime{};
  localtime_s(&LocalTime, &Time);
  char Buffer[64]{};
  std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%d_%H-%M-%S", &LocalTime);
  return std::filesystem::path("Logs") / (std::string(Buffer) + ".log");
}

bool PassesLevel(ELogLevel Level, const std::optional<ELogLevel>& MinimumLevel) {
  return !MinimumLevel || static_cast<uint8_t>(Level) >= static_cast<uint8_t>(*MinimumLevel);
}
}  // namespace

void MLog::Write(ELogLevel Level, std::string_view Category, std::string_view Message) noexcept {
  std::string SafeCategory;
  std::string SafeMessage;
  try {
    SafeCategory = TruncateUtf8(
        Category.empty() ? std::string_view("Unknown") : Category, MaxLogCategoryBytes
    );
    SafeMessage = TruncateUtf8(Message, MaxLogMessageBytes);
    FLogEntry Entry;
    Entry.Timestamp = std::chrono::system_clock::now();
    Entry.Level = Level;
    Entry.Category = SafeCategory;
    Entry.Message = SafeMessage;

    FMLogState& State = GetLogState();
    {
      std::scoped_lock Lock(State.BufferMutex);
      if (State.NextSequence == 0) {
        throw std::overflow_error("Log sequence exhausted");
      }
      Entry.Sequence = State.NextSequence;
      State.NextSequence =
          State.NextSequence == (std::numeric_limits<uint64_t>::max)() ? 0 : State.NextSequence + 1;
      if (State.Entries.size() == MaxLogEntries) {
        State.Entries.pop_front();
      }
      State.Entries.push_back(std::move(Entry));
    }
  } catch (...) {
  }

  try {
    const std::string SinkMessage = std::format("[{}] {}\n", SafeCategory, SafeMessage);
    FMLogState& State = GetLogState();
    std::scoped_lock Lock(State.SinkMutex);

    const std::wstring WideMessage = Utf8ToWide(SinkMessage);
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
        std::cout << SinkMessage;
      }
    } else {
      OutputDebugStringA(SinkMessage.c_str());
      std::cout << SinkMessage;
    }

    if (State.LogFilePath.empty()) {
      State.LogFilePath = CreateLogFilePath();
    }
    std::ofstream LogFile(State.LogFilePath, std::ios::app);
    if (LogFile.is_open()) {
      LogFile << SinkMessage;
    }
  } catch (const std::exception& Error) {
    std::cerr << "[MLog Error] " << Error.what() << std::endl;
  } catch (...) {
    std::cerr << "[MLog Error] Unknown logging failure." << std::endl;
  }
}

FLogQueryResult MLog::GetRecentEntries(const FLogQuery& Query) {
  if (Query.Limit == 0 || Query.Limit > MaxLogEntries) {
    throw std::invalid_argument("Log query limit is out of range");
  }

  FLogQueryResult Result;
  FMLogState& State = GetLogState();
  {
    std::scoped_lock Lock(State.BufferMutex);
    if (!State.Entries.empty()) {
      Result.OldestAvailableSequence = State.Entries.front().Sequence;
      Result.LatestSequence = State.Entries.back().Sequence;
    }

    if (Query.AfterSequence) {
      const uint64_t AfterSequence = *Query.AfterSequence;
      Result.NextAfterSequence = AfterSequence;
      Result.bHistoryLost = AfterSequence < Result.OldestAvailableSequence &&
                            Result.OldestAvailableSequence - AfterSequence > 1;
      for (const FLogEntry& Entry : State.Entries) {
        if (Entry.Sequence <= AfterSequence || !PassesLevel(Entry.Level, Query.MinimumLevel)) {
          continue;
        }
        if (Result.Entries.size() == Query.Limit) {
          Result.bHasMore = true;
          break;
        }
        Result.Entries.push_back(Entry);
      }
    } else {
      for (auto Iterator = State.Entries.rbegin(); Iterator != State.Entries.rend(); ++Iterator) {
        if (!PassesLevel(Iterator->Level, Query.MinimumLevel)) {
          continue;
        }
        if (Result.Entries.size() == Query.Limit) {
          Result.bHasMore = true;
          break;
        }
        Result.Entries.push_back(*Iterator);
      }
      std::reverse(Result.Entries.begin(), Result.Entries.end());
    }
  }

  if (!Result.Entries.empty()) {
    Result.NextAfterSequence = Result.Entries.back().Sequence;
  }
  return Result;
}

std::string_view MLog::ToLevelString(ELogLevel Level) {
  switch (Level) {
    case ELogLevel::Debug:
      return "debug";
    case ELogLevel::Info:
      return "info";
    case ELogLevel::Warning:
      return "warning";
    case ELogLevel::Error:
      return "error";
  }
  return "info";
}

std::string MLog::FormatTimestamp(const std::chrono::system_clock::time_point& Timestamp) {
  const auto Milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(Timestamp.time_since_epoch());
  const auto WholeSeconds = std::chrono::duration_cast<std::chrono::seconds>(Milliseconds);
  const int MillisecondPart = static_cast<int>((Milliseconds - WholeSeconds).count());
  const std::time_t Time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(WholeSeconds));
  std::tm UtcTime{};
  gmtime_s(&UtcTime, &Time);

  char Buffer[32]{};
  const int Length = std::snprintf(
      Buffer,
      sizeof(Buffer),
      "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
      UtcTime.tm_year + 1900,
      UtcTime.tm_mon + 1,
      UtcTime.tm_mday,
      UtcTime.tm_hour,
      UtcTime.tm_min,
      UtcTime.tm_sec,
      MillisecondPart
  );
  if (Length <= 0 || static_cast<size_t>(Length) >= sizeof(Buffer)) {
    throw std::runtime_error("Failed to format log timestamp");
  }
  return std::string(Buffer, static_cast<size_t>(Length));
}
