#include "BuiltInCommands.h"

#include "NetworkTypes.h"
#include "Registration/SystemCommandBinding.h"
#include "Registry/SystemCommandRegistry.h"
#include "Runtime/RuntimeState.h"
#include "SceneManager.h"
#include "World.h"

void RegisterAutomationBuiltInCommands(
    FAutomationSystemCommandRegistry& Registry, FAutomationRuntimeState& RuntimeState
) {
  AutomationHelper::RegisterSystemCommand(
      Registry,
      "pause_game",
      "Pause world updates while keeping automation available.",
      [&RuntimeState]() {
        const bool Changed = !RuntimeState.bPaused;
        RuntimeState.bPaused = true;
        M_LOG(
            Log,
            "Automation system command state changed: command=pause_game changed={} paused=true",
            Changed
        );
        return nlohmann::json{
            {"commandName", "pause_game"}, {"changed", Changed}, {"paused", true}
        };
      }
  );
  AutomationHelper::RegisterSystemCommand(
      Registry, "resume_game", "Resume world updates.", [&RuntimeState]() {
        const bool Changed = RuntimeState.bPaused;
        RuntimeState.bPaused = false;
        M_LOG(
            Log,
            "Automation system command state changed: command=resume_game changed={} paused=false",
            Changed
        );
        return nlohmann::json{
            {"commandName", "resume_game"}, {"changed", Changed}, {"paused", false}
        };
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
