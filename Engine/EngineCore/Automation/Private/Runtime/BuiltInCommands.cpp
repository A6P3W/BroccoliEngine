#include "BuiltInCommands.h"

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
      [](FNetworkSceneId SceneId) {
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(SceneId)
                                         : Manager.OpenLevelById(SceneId, ENetMode::Standalone);
        return nlohmann::json{
            {"commandName", "open_level_by_id"}, {"sceneId", SceneId}, {"queued", Queued}
        };
      },
      AutomationParam<FNetworkSceneId>("sceneId", "Registered scene ID.")
  );
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "open_level_by_path",
      "Queue a level to open by file path.",
      [](std::string LevelPath) {
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(LevelPath)
                                         : Manager.OpenLevelByPath(LevelPath, ENetMode::Standalone);
        return nlohmann::json{
            {"commandName", "open_level_by_path"}, {"levelPath", LevelPath}, {"queued", Queued}
        };
      },
      AutomationParam<std::string>("levelPath", "Level file path.")
  );
}
