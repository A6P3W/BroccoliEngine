#include "BuiltInCommands.h"

#include <optional>
#include <string_view>

#include "Application.h"
#include "NetworkTypes.h"
#include "Registration/SystemCommandBinding.h"
#include "Registry/SystemCommandRegistry.h"
#include "SceneManager.h"
#include "World.h"

namespace {
nlohmann::json SetSimulationState(std::string_view CommandName, bool Simulating) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return nlohmann::json{
        {"commandName", CommandName},
        {"worldAvailable", false},
        {"changed", false},
        {"simulating", false}
    };
  }

  const bool Changed = CurrentWorld->IsSimulating() != Simulating;
  CurrentWorld->SetSimulating(Simulating);
  M_LOG(
      Log,
      "Automation system command state changed: command={} changed={} simulating={}",
      CommandName,
      Changed,
      Simulating
  );
  return nlohmann::json{
      {"commandName", CommandName},
      {"worldAvailable", true},
      {"changed", Changed},
      {"simulating", Simulating}
  };
}
}  // namespace

void RegisterAutomationBuiltInCommands(FAutomationSystemCommandRegistry& Registry) {
  AutomationHelper::RegisterSystemCommand(
      Registry, "stop_engine_update", "Stop Engine updates at an update boundary.", [] {
        return Application::GetUpdateControl()->Stop();
      }
  );
  AutomationHelper::RegisterSystemCommand(
      Registry, "start_engine_update", "Resume automatic Engine updates.", [] {
        return Application::GetUpdateControl()->Start();
      }
  );
  FAutomationSystemCommandDescriptor StepDescriptor;
  StepDescriptor.Name = "step_engine_update";
  StepDescriptor.Description = "Run count Engine updates with fixed delta time.";
  StepDescriptor.InputSchema = {
      {"type", "object"},
      {"properties", {{"count", {{"type", "integer"}, {"minimum", 1}, {"maximum", 1000000}}}}},
      {"required", {"count"}},
      {"additionalProperties", false}
  };
  StepDescriptor.Handler = [](const nlohmann::json& Arguments) {
    return Application::GetUpdateControl()->Step(Arguments.at("count").get<uint32_t>());
  };
  AutomationHelper::Detail::RegisterSystemCommandDescriptor(Registry, std::move(StepDescriptor));
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "operation_status",
      "Query a retained asynchronous operation.",
      [](std::string OperationId) {
        return Application::GetUpdateControl()->GetOperation(OperationId);
      },
      AutomationParam<std::string>("operationId", "Operation ID returned at acceptance.")
  );
  AutomationHelper::RegisterSystemCommand(
      Registry, "start_simulation", "Start world simulation.", [] {
        return SetSimulationState("start_simulation", true);
      }
  );
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "stop_simulation",
      "Stop world simulation while keeping world updates available.",
      [] { return SetSimulationState("stop_simulation", false); }
  );
  AutomationHelper::RegisterSystemCommand(
      Registry, "quit_game", "Request normal game shutdown.", [] {
        Application::QuitGame();
        return nlohmann::json{{"quitting", true}};
      }
  );
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "open_level_by_id",
      "Queue a registered level to open by scene ID.",
      [](FNetworkSceneId SceneId, std::optional<std::string> AfterUpdate) {
        auto* Control = Application::GetUpdateControl();
        const std::string After = AfterUpdate.value_or("preserve");
        if (After != "preserve" && After != "stop" && After != "run") {
          return MakeAutomationError(EAutomationErrorCode::InvalidArgument, "Invalid afterUpdate.");
        }
        if (!Control || !Control->CanOpenLevel()) {
          return MakeAutomationError(
              EAutomationErrorCode::Conflict,
              "Level transitions require a running Engine with no active operation."
          );
        }
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(SceneId)
                                         : Manager.OpenLevelById(SceneId, ENetMode::Standalone);
        if (!Queued)
          return MakeAutomationError(
              EAutomationErrorCode::InvalidArgument, "Level could not be queued."
          );
        auto Result = Control->TrackLevel(Manager.GetPendingRequestId(), After);
        Result.update(
            {{"commandName", "open_level_by_id"}, {"sceneId", SceneId}, {"queued", true}}
        );
        return Result;
      },
      AutomationParam<FNetworkSceneId>("sceneId", "Registered scene ID."),
      AutomationOptionalParam<std::string>("afterUpdate", "preserve, stop, or run after loading.")
  );
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "open_level_by_path",
      "Queue a level to open by file path.",
      [](std::string LevelPath, std::optional<std::string> AfterUpdate) {
        auto* Control = Application::GetUpdateControl();
        const std::string After = AfterUpdate.value_or("preserve");
        if (After != "preserve" && After != "stop" && After != "run") {
          return MakeAutomationError(EAutomationErrorCode::InvalidArgument, "Invalid afterUpdate.");
        }
        if (!Control || !Control->CanOpenLevel()) {
          return MakeAutomationError(
              EAutomationErrorCode::Conflict,
              "Level transitions require a running Engine with no active operation."
          );
        }
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(LevelPath)
                                         : Manager.OpenLevelByPath(LevelPath, ENetMode::Standalone);
        if (!Queued)
          return MakeAutomationError(
              EAutomationErrorCode::InvalidArgument, "Level could not be queued."
          );
        auto Result = Control->TrackLevel(Manager.GetPendingRequestId(), After);
        Result.update(
            {{"commandName", "open_level_by_path"}, {"levelPath", LevelPath}, {"queued", true}}
        );
        return Result;
      },
      AutomationParam<std::string>("levelPath", "Level file path."),
      AutomationOptionalParam<std::string>("afterUpdate", "preserve, stop, or run after loading.")
  );
}
