#include <cassert>
#include <cstdint>
#include <string>

#include "EngineUpdateControl.h"

namespace {
std::string OperationId(const nlohmann::json& Operation) {
  return Operation.at("operationId").get<std::string>();
}

void TestStopAndStartDoNotAdvanceUpdates() {
  FEngineUpdateControl Control;
  Control.Configure(true, 1.0f / 30.0f);

  const auto Stop = Control.Stop();
  assert(Stop.at("status") == "completed");
  assert(Control.GetState().at("completedEngineUpdates") == 0);
  assert(!Control.BeginUpdate());

  const auto Start = Control.Start();
  assert(Start.at("engineUpdate") == "running");
  assert(Control.TakeTimeReset());
  assert(!Control.TakeTimeReset());
  assert(Control.GetState().at("completedEngineUpdates") == 0);
}

void TestStepConsumesOnlyUpdatesStartedForThatOperation() {
  FEngineUpdateControl Control;
  Control.Configure(false, 0.025f);
  const auto Step = Control.Step(2);
  const auto StepId = OperationId(Step);

  assert(Control.BeginUpdate());
  assert(Control.IsStepUpdate());
  assert(Control.GetStepDeltaTime() == 0.025f);
  Control.CompleteUpdate();
  auto Status = Control.GetOperation(StepId);
  assert(Status.at("status") == "running");
  assert(Status.at("completedCount") == 1);

  assert(Control.BeginUpdate());
  Control.CompleteUpdate();
  Status = Control.GetOperation(StepId);
  assert(Status.at("status") == "completed");
  assert(Status.at("completedCount") == 2);
  assert(Status.at("endUpdate") == 2);
  assert(!Control.BeginUpdate());
}

void TestStopCancelsStepAfterCurrentUpdate() {
  FEngineUpdateControl Control;
  Control.Configure(false, 1.0f / 60.0f);
  const auto Step = Control.Step(5);
  const auto StepId = OperationId(Step);
  assert(Control.BeginUpdate());

  const auto Stop = Control.Stop();
  const auto StopId = OperationId(Stop);
  assert(Control.GetOperation(StopId).at("status") == "pending");
  Control.CompleteUpdate();

  const auto CancelledStep = Control.GetOperation(StepId);
  assert(CancelledStep.at("status") == "cancelled");
  assert(CancelledStep.at("completedCount") == 1);
  assert(CancelledStep.at("endUpdate") == 1);
  assert(Control.GetOperation(StopId).at("status") == "completed");
  assert(!Control.BeginUpdate());
}

void TestInvalidStepsAndConflictingOperationsAreRejected() {
  FEngineUpdateControl Control;
  Control.Configure(true, 1.0f / 60.0f);
  assert(!Control.Step(0).at("success").get<bool>());
  assert(!Control.Step(1000001).at("success").get<bool>());
  assert(!Control.Step(1).at("success").get<bool>());

  Control.Stop();
  assert(Control.Step(1).at("status") == "pending");
}

void TestLevelCompletionMustMatchRequestBeforeStopping() {
  FEngineUpdateControl Control;
  Control.Configure(true, 1.0f / 60.0f);
  const auto Level = Control.TrackLevel(42, "stop");
  const auto LevelId = OperationId(Level);

  assert(!Control.OnSceneProcessed(41, true));
  assert(Control.GetOperation(LevelId).at("status") == "pending");
  assert(Control.GetState().at("levelTransitionPending").get<bool>());

  assert(Control.OnSceneProcessed(42, true));
  assert(Control.GetOperation(LevelId).at("status") == "completed");
  assert(Control.GetState().at("engineUpdate") == "stopped");
  assert(!Control.GetState().at("levelTransitionPending").get<bool>());
}

void TestFailedLevelDoesNotStopAutomaticUpdates() {
  FEngineUpdateControl Control;
  Control.Configure(true, 1.0f / 60.0f);
  const auto Level = Control.TrackLevel(90, "stop");

  assert(!Control.OnSceneProcessed(90, false));
  assert(Control.GetOperation(OperationId(Level)).at("status") == "failed");
  assert(Control.GetState().at("engineUpdate") == "running");
  assert(!Control.GetState().at("levelTransitionPending").get<bool>());
}

void TestRepeatedStartKeepsTimeResetPending() {
  FEngineUpdateControl Control;
  Control.Configure(false, 1.0f / 60.0f);

  Control.Start();
  Control.Start();
  assert(Control.TakeTimeReset());
  assert(!Control.TakeTimeReset());
}
}  // namespace

int main() {
  TestStopAndStartDoNotAdvanceUpdates();
  TestStepConsumesOnlyUpdatesStartedForThatOperation();
  TestStopCancelsStepAfterCurrentUpdate();
  TestInvalidStepsAndConflictingOperationsAreRejected();
  TestLevelCompletionMustMatchRequestBeforeStopping();
  TestFailedLevelDoesNotStopAutomaticUpdates();
  TestRepeatedStartKeepsTimeResetPending();
}
