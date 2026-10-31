#pragma once

#include <cstdint>
#include <deque>
#include <nlohmann/json.hpp>
#include <string>

#include "BroccoliEngineAPI.h"

// Main-thread execution state. A frame captures its step before processing control commands.
class BROCCOLI_ENGINE_API FEngineUpdateControl {
 public:
  void Configure(bool Running, float FixedDeltaTime);
  bool BeginUpdate();
  void CompleteUpdate();
  void SkipUpdate();
  void Shutdown();
  void RequestShutdown() { ShuttingDown = true; }
  bool IsShuttingDown() const { return ShuttingDown; }
  bool IsStepping() const { return !StepOperation.empty(); }
  bool CanMutate() const { return !ShuttingDown && !IsStepping(); }
  bool IsRunning() const { return AutomaticUpdates; }
  bool IsUpdating() const { return Updating; }
  bool IsStepUpdate() const { return !FrameStepOperation.empty(); }
  float GetStepDeltaTime() const { return StepDeltaTime; }
  bool TakeTimeReset();

  nlohmann::json Stop();
  nlohmann::json Start();
  nlohmann::json Step(uint32_t Count);
  nlohmann::json GetOperation(const std::string& OperationId) const;
  nlohmann::json GetState() const;
  bool CanOpenLevel() const;
  nlohmann::json TrackLevel(uint64_t RequestId, const std::string& AfterUpdate);
  bool OnSceneProcessed(uint64_t RequestId, bool Success);
  void FailSupersededLevel(uint64_t PendingRequestId);

 private:
  nlohmann::json& NewOperation(const std::string& Kind, uint32_t Count = 0);
  nlohmann::json* FindOperation(const std::string& OperationId);
  void Finish(
      const std::string& OperationId, const std::string& Status, const std::string& Reason = ""
  );
  void SettleStop();

  bool ShuttingDown = false;
  bool AutomaticUpdates = true;
  bool Updating = false;
  bool ResetTime = false;
  bool StopAfterLevel = false;
  float StepDeltaTime = 1.0f / 60.0f;
  uint64_t CompletedUpdates = 0;
  uint64_t SceneGeneration = 0;
  uint64_t NextOperationId = 1;
  uint64_t LevelRequestId = 0;
  std::string StepOperation;
  std::string FrameStepOperation;
  std::string StopOperation;
  std::string LevelOperation;
  std::deque<nlohmann::json> Operations;
};
