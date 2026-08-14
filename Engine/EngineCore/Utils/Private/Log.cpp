#include "Log.h"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
constexpr size_t MaxLogEntries = 1000;
constexpr size_t MaxQueuedLogs = 8192;
constexpr size_t MaxLogMessageBytes = 4096;
constexpr size_t MaxLogCategoryBytes = 128;
constexpr auto LogFlushInterval = std::chrono::seconds(5);

enum class ELogQueueCommand : uint8_t { Entry, FlushBarrier, Shutdown };

struct FLogQueueItem {
  ELogQueueCommand Command = ELogQueueCommand::Entry;
  FLogEntry Entry;
  std::shared_ptr<std::promise<void>> Completion;
};

struct FMLogState {
  std::mutex BufferMutex;
  std::deque<FLogEntry> Entries;
  std::atomic<uint64_t> NextSequence = 1;
  std::atomic<uint64_t> DroppedLogEntries = 0;

  std::mutex QueueMutex;
  std::condition_variable QueueCondition;
  std::condition_variable QueueSpaceCondition;
  std::deque<FLogQueueItem> Queue;
  std::thread Worker;
  std::mutex LifecycleMutex;
  bool Running = false;
  bool ShutdownInProgress = false;
  bool AcceptingLogs = false;
  std::atomic<bool> Stopped = false;
  std::atomic<bool> PendingWarningFlush = false;

  std::filesystem::path LogFilePath;
  std::ofstream LogFile;
  std::chrono::steady_clock::time_point LastFlushTime;
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

bool RemoveQueuedLogEntry(FMLogState& State) {
  const auto Iterator =
      std::find_if(State.Queue.begin(), State.Queue.end(), [](const FLogQueueItem& Item) {
        return Item.Command == ELogQueueCommand::Entry && Item.Entry.Level == ELogLevel::Log;
      });
  if (Iterator == State.Queue.end()) return false;
  State.Queue.erase(Iterator);
  ++State.DroppedLogEntries;
  return true;
}

void FlushFile(FMLogState& State) {
  if (State.LogFile.is_open()) State.LogFile.flush();
  State.LastFlushTime = std::chrono::steady_clock::now();
}

void WriteEntry(FMLogState& State, const FLogEntry& Entry) {
  const std::string SinkMessage = std::format("[{}] {}\n", Entry.Category, Entry.Message);
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

  if (State.LogFile.is_open()) State.LogFile << SinkMessage;
}

void WorkerMain() {
  FMLogState& State = GetLogState();
  try {
    State.LogFilePath = CreateLogFilePath();
    State.LogFile.open(State.LogFilePath, std::ios::out | std::ios::app);
  } catch (...) {
  }
  State.LastFlushTime = std::chrono::steady_clock::now();

  while (true) {
    FLogQueueItem Item;
    {
      std::unique_lock Lock(State.QueueMutex);
      State.QueueCondition.wait_for(Lock, LogFlushInterval, [&State] {
        return !State.Queue.empty();
      });
      if (State.Queue.empty()) {
        Lock.unlock();
        FlushFile(State);
        continue;
      }
      Item = std::move(State.Queue.front());
      State.Queue.pop_front();
      State.QueueSpaceCondition.notify_all();
    }

    if (Item.Command == ELogQueueCommand::Shutdown) {
      FlushFile(State);
      break;
    }
    if (Item.Command == ELogQueueCommand::FlushBarrier) {
      FlushFile(State);
      if (Item.Completion) {
        try {
          Item.Completion->set_value();
        } catch (...) {
        }
      }
      continue;
    }

    try {
      WriteEntry(State, Item.Entry);
    } catch (...) {
    }
    if (std::chrono::steady_clock::now() - State.LastFlushTime >= LogFlushInterval)
      FlushFile(State);
  }

  FlushFile(State);
  if (State.LogFile.is_open()) State.LogFile.close();
}

bool EnqueueInternal(FMLogState& State, std::unique_lock<std::mutex>& Lock, FLogQueueItem&& Item) {
  if (!State.AcceptingLogs) return false;

  const bool IsNormalLog =
      Item.Command == ELogQueueCommand::Entry && Item.Entry.Level == ELogLevel::Log;
  while (State.Queue.size() >= MaxQueuedLogs) {
    if (IsNormalLog) {
      ++State.DroppedLogEntries;
      return false;
    }
    if (RemoveQueuedLogEntry(State)) break;
    State.QueueSpaceCondition.wait(Lock, [&State] {
      return State.Queue.size() < MaxQueuedLogs || !State.AcceptingLogs;
    });
    if (!State.AcceptingLogs) return false;
  }
  State.Queue.push_back(std::move(Item));
  Lock.unlock();
  State.QueueCondition.notify_one();
  return true;
}

bool Enqueue(FLogQueueItem&& Item) {
  FMLogState& State = GetLogState();
  std::unique_lock Lock(State.QueueMutex);
  return EnqueueInternal(State, Lock, std::move(Item));
}

bool EnqueueEntry(FLogEntry&& Entry) {
  FMLogState& State = GetLogState();
  std::unique_lock Lock(State.QueueMutex);
  if (!State.AcceptingLogs) return false;

  if (Entry.Level == ELogLevel::Log && State.Queue.size() >= MaxQueuedLogs) {
    ++State.DroppedLogEntries;
    return false;
  }

  Entry.Sequence = State.NextSequence.fetch_add(1);
  if (Entry.Sequence == 0) return false;
  {
    std::scoped_lock BufferLock(State.BufferMutex);
    if (State.Entries.size() == MaxLogEntries) State.Entries.pop_front();
    State.Entries.push_back(Entry);
  }

  return EnqueueInternal(
      State, Lock, FLogQueueItem{ELogQueueCommand::Entry, std::move(Entry), {}}
  );
}

void EnqueueFlushBarrierAndWait() {
  FMLogState& State = GetLogState();
  if (State.Stopped.load(std::memory_order_relaxed)) return;

  auto Completion = std::make_shared<std::promise<void>>();
  std::future<void> Future = Completion->get_future();
  if (!Enqueue({ELogQueueCommand::FlushBarrier, {}, Completion})) return;
  try {
    Future.wait();
  } catch (...) {
  }
}
}  // namespace

void MLog::Write(ELogLevel Level, std::string_view Category, std::string_view Message) noexcept {
  FMLogState& State = GetLogState();
  if (State.Stopped.load(std::memory_order_relaxed)) return;

  std::string SafeCategory;
  std::string SafeMessage;
  FLogEntry Entry;
  try {
    SafeCategory = TruncateUtf8(
        Category.empty() ? std::string_view("Unknown") : Category, MaxLogCategoryBytes
    );
    SafeMessage = TruncateUtf8(Message, MaxLogMessageBytes);
    Entry.Timestamp = std::chrono::system_clock::now();
    Entry.Level = Level;
    Entry.Category = SafeCategory;
    Entry.Message = SafeMessage;

    Initialize();
  } catch (...) {
    return;
  }

  if (!EnqueueEntry(std::move(Entry))) return;

  if (Level == ELogLevel::Warning || Level == ELogLevel::Error) {
    State.PendingWarningFlush.store(true, std::memory_order_relaxed);
  }
}

void MLog::Initialize() {
  FMLogState& State = GetLogState();
  if (State.Stopped.load(std::memory_order_relaxed)) return;
  std::scoped_lock Lock(State.LifecycleMutex);
  if (State.Running || State.Stopped.load(std::memory_order_relaxed)) return;
  {
    std::scoped_lock QueueLock(State.QueueMutex);
    State.AcceptingLogs = true;
  }
  State.Running = true;
  State.Worker = std::thread(WorkerMain);
}

void MLog::Shutdown() {
  FMLogState& State = GetLogState();
  {
    std::scoped_lock LifecycleLock(State.LifecycleMutex);
    if (!State.Running || State.ShutdownInProgress) {
      State.Stopped.store(true, std::memory_order_relaxed);
      return;
    }
    State.ShutdownInProgress = true;
    State.Stopped.store(true, std::memory_order_relaxed);
    {
      std::scoped_lock QueueLock(State.QueueMutex);
      State.AcceptingLogs = false;
      State.Queue.push_back({ELogQueueCommand::Shutdown, {}, {}});
    }
    State.QueueCondition.notify_one();
    State.QueueSpaceCondition.notify_all();
  }
  if (State.Worker.joinable()) State.Worker.join();

  {
    std::scoped_lock LifecycleLock(State.LifecycleMutex);
    std::scoped_lock QueueLock(State.QueueMutex);
    for (auto& Item : State.Queue) {
      if (Item.Command == ELogQueueCommand::FlushBarrier && Item.Completion) {
        try {
          Item.Completion->set_value();
        } catch (...) {
        }
      }
    }
    State.Queue.clear();
    State.Running = false;
    State.ShutdownInProgress = false;
  }
}

void MLog::EndFrame() {
  FMLogState& State = GetLogState();
  if (State.PendingWarningFlush.exchange(false, std::memory_order_relaxed)) {
    Enqueue({ELogQueueCommand::FlushBarrier, {}, {}});
  }
}

void MLog::Flush() { EnqueueFlushBarrierAndWait(); }

FLogQueryResult MLog::GetRecentEntries(const FLogQuery& Query) {
  if (Query.Limit == 0 || Query.Limit > MaxLogEntries) {
    throw std::invalid_argument("Log query limit is out of range");
  }

  FLogQueryResult Result;
  FMLogState& State = GetLogState();
  Result.DroppedEntries = State.DroppedLogEntries.load();
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
      Result.bHistoryLost = false;
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
    case ELogLevel::Log:
      return "log";
    case ELogLevel::Warning:
      return "warning";
    case ELogLevel::Error:
      return "error";
  }
  return "log";
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
