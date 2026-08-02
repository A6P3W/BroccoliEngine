#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string_view>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

using FAutomationTask = std::function<nlohmann::json()>;

struct FAutomationCommand {
  uint64_t RequestId = 0;
  FAutomationTask Execute;
  std::promise<nlohmann::json> ResultPromise;
  std::mutex StateMutex;
  std::atomic_bool bCancelled = false;
  std::atomic_bool bStarted = false;
  std::atomic_bool bCompleted = false;
};

using FAutomationCommandPtr = std::shared_ptr<FAutomationCommand>;

struct FAutomationCommandTicket {
  uint64_t RequestId = 0;
  FAutomationCommandPtr Command;
  std::future<nlohmann::json> Result;

  bool IsValid() const;
  void Cancel();
};

class BROCCOLI_ENGINE_API FAutomationCommandQueue {
 public:
  FAutomationCommandQueue() = default;
  ~FAutomationCommandQueue();

  FAutomationCommandQueue(const FAutomationCommandQueue&) = delete;
  FAutomationCommandQueue& operator=(const FAutomationCommandQueue&) = delete;

  FAutomationCommandTicket Enqueue(FAutomationTask Task);

  void ProcessCommands();
  void StopAcceptingCommands();
  void CancelAll(EAutomationErrorCode ErrorCode, std::string_view Message);

  size_t GetPendingCommandCount() const;
  bool IsAcceptingCommands() const;

 private:
  void CompleteCommand(const FAutomationCommandPtr& Command, nlohmann::json Result);

  mutable std::mutex QueueMutex;
  std::deque<FAutomationCommandPtr> PendingCommands;
  std::atomic_uint64_t NextRequestId = 1;
  std::atomic_bool bAcceptingCommands = true;
};
