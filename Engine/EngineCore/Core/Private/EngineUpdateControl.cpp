#include "EngineUpdateControl.h"

#include "AutomationTypes.h"

namespace {
nlohmann::json Conflict(const std::string& Message) {
  return MakeAutomationError(EAutomationErrorCode::Conflict, Message);
}
}  // namespace

void FEngineUpdateControl::Configure(bool Running, float FixedDeltaTime) {
  AutomaticUpdates = Running;
  StepDeltaTime = FixedDeltaTime;
}

nlohmann::json& FEngineUpdateControl::NewOperation(const std::string& Kind, uint32_t Count) {
  // At most one step/level and one stop are active. Never evict an unfinished operation.
  if (Operations.size() >= 256) {
    for (auto Iterator = Operations.begin(); Iterator != Operations.end(); ++Iterator) {
      const auto Status = (*Iterator)["status"].get<std::string>();
      if (Status != "pending" && Status != "running") {
        Operations.erase(Iterator);
        break;
      }
    }
  }
  Operations.push_back(
      {{"operationId", std::to_string(NextOperationId++)},
       {"kind", Kind},
       {"status", "pending"},
       {"requestedCount", Count},
       {"completedCount", 0},
       {"startUpdate", CompletedUpdates},
       {"endUpdate", nullptr},
       {"reason", ""}}
  );
  return Operations.back();
}

nlohmann::json* FEngineUpdateControl::FindOperation(const std::string& OperationId) {
  for (auto& Operation : Operations) {
    if (Operation["operationId"] == OperationId) return &Operation;
  }
  return nullptr;
}

nlohmann::json FEngineUpdateControl::GetOperation(const std::string& OperationId) const {
  for (const auto& Operation : Operations) {
    if (Operation["operationId"] == OperationId) return Operation;
  }
  return MakeAutomationError(
      EAutomationErrorCode::InvalidArgument, "Unknown or expired operationId."
  );
}

void FEngineUpdateControl::Finish(
    const std::string& OperationId, const std::string& Status, const std::string& Reason
) {
  if (auto* Operation = FindOperation(OperationId)) {
    (*Operation)["status"] = Status;
    (*Operation)["endUpdate"] = CompletedUpdates;
    (*Operation)["reason"] = Reason;
  }
}

bool FEngineUpdateControl::BeginUpdate() {
  if (ShuttingDown || Updating || (!AutomaticUpdates && StepOperation.empty())) return false;
  Updating = true;
  FrameStepOperation = StepOperation;
  if (auto* Operation = FindOperation(FrameStepOperation)) {
    if ((*Operation)["status"] == "pending") (*Operation)["startUpdate"] = CompletedUpdates;
    (*Operation)["status"] = "running";
  }
  return true;
}

void FEngineUpdateControl::SettleStop() {
  if (StopOperation.empty()) return;
  if (!StepOperation.empty()) {
    Finish(StepOperation, "cancelled", "Stopped by control.");
    StepOperation.clear();
  }
  Finish(StopOperation, "completed");
  StopOperation.clear();
}

void FEngineUpdateControl::CompleteUpdate() {
  ++CompletedUpdates;
  if (!FrameStepOperation.empty() && FrameStepOperation == StepOperation) {
    auto* Operation = FindOperation(FrameStepOperation);
    (*Operation)["completedCount"] = (*Operation)["completedCount"].get<uint32_t>() + 1;
    if (StopOperation.empty() && (*Operation)["completedCount"] == (*Operation)["requestedCount"]) {
      Finish(StepOperation, "completed");
      StepOperation.clear();
    }
  }
  Updating = false;
  FrameStepOperation.clear();
  SettleStop();
}

void FEngineUpdateControl::SkipUpdate() {
  Updating = false;
  FrameStepOperation.clear();
  SettleStop();
}

nlohmann::json FEngineUpdateControl::Stop() {
  if (ShuttingDown)
    return MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, "Engine shutting down.");
  if (!LevelOperation.empty()) return Conflict("A level transition is pending.");
  if (!StopOperation.empty()) return GetOperation(StopOperation);
  AutomaticUpdates = false;
  auto& Operation = NewOperation("stop");
  StopOperation = Operation["operationId"].get<std::string>();
  const std::string OperationId = StopOperation;
  if (!Updating) SettleStop();
  return GetOperation(OperationId);
}

nlohmann::json FEngineUpdateControl::Start() {
  if (ShuttingDown)
    return MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, "Engine shutting down.");
  if (IsStepping() || !StopOperation.empty()) return Conflict("An update operation is active.");
  if (!AutomaticUpdates) ResetTime = true;
  AutomaticUpdates = true;
  return {{"engineUpdate", "running"}};
}

nlohmann::json FEngineUpdateControl::Step(uint32_t Count) {
  if (ShuttingDown)
    return MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, "Engine shutting down.");
  if (Count == 0 || Count > 1000000) {
    return MakeAutomationError(
        EAutomationErrorCode::InvalidArgument, "count must be between 1 and 1000000."
    );
  }
  if (AutomaticUpdates || IsStepping() || !StopOperation.empty() || !LevelOperation.empty()) {
    return Conflict("Engine must be stopped with no active operation before stepping.");
  }
  auto& Operation = NewOperation("step", Count);
  StepOperation = Operation["operationId"].get<std::string>();
  return Operation;
}

bool FEngineUpdateControl::TakeTimeReset() {
  const bool Result = ResetTime;
  ResetTime = false;
  return Result;
}

bool FEngineUpdateControl::CanOpenLevel() const {
  return !ShuttingDown && AutomaticUpdates && !IsStepping() && StopOperation.empty() &&
         LevelOperation.empty();
}

nlohmann::json FEngineUpdateControl::TrackLevel(
    uint64_t RequestId, const std::string& AfterUpdate
) {
  auto& Operation = NewOperation("level");
  LevelOperation = Operation["operationId"].get<std::string>();
  LevelRequestId = RequestId;
  StopAfterLevel = AfterUpdate == "stop";
  Operation["afterUpdate"] = AfterUpdate;
  return Operation;
}

bool FEngineUpdateControl::OnSceneProcessed(uint64_t RequestId, bool Success) {
  if (Success) ++SceneGeneration;
  if (LevelOperation.empty() || LevelRequestId != RequestId) return false;
  const bool StopAtBoundary = Success && StopAfterLevel;
  if (StopAtBoundary) AutomaticUpdates = false;
  Finish(
      LevelOperation,
      Success ? "completed" : "failed",
      Success ? "" : "Level load failed; previous scene preserved."
  );
  LevelOperation.clear();
  LevelRequestId = 0;
  return StopAtBoundary;
}

void FEngineUpdateControl::FailSupersededLevel(uint64_t PendingRequestId) {
  if (!LevelOperation.empty() && LevelRequestId != PendingRequestId) {
    Finish(LevelOperation, "failed", "Level request superseded before loading.");
    LevelOperation.clear();
    LevelRequestId = 0;
  }
}

nlohmann::json FEngineUpdateControl::GetState() const {
  return {
      {"engineUpdate", AutomaticUpdates ? "running" : "stopped"},
      {"engineStopped", !AutomaticUpdates && !Updating && !IsStepping()},
      {"stepping", IsStepping()},
      {"stepDeltaTime", StepDeltaTime},
      {"completedEngineUpdates", CompletedUpdates},
      {"sceneGeneration", SceneGeneration},
      {"activeOperationId", !StepOperation.empty() ? StepOperation : StopOperation},
      {"levelOperationId", LevelOperation},
      {"levelTransitionPending", !LevelOperation.empty()}
  };
}

void FEngineUpdateControl::Shutdown() {
  ShuttingDown = true;
  for (auto& Operation : Operations) {
    if (Operation["status"] == "pending" || Operation["status"] == "running") {
      Operation["status"] = "cancelled";
      Operation["reason"] = "Engine shutting down.";
      Operation["endUpdate"] = CompletedUpdates;
    }
  }
  AutomaticUpdates = false;
  StepOperation.clear();
  StopOperation.clear();
  LevelOperation.clear();
}
