#include "AutomationStateProvider.h"

#include <filesystem>

#include "ActorManager.h"
#include "SceneManager.h"
#include "World.h"

namespace {
std::string GetCurrentSceneName(const SceneManager& Manager) {
  const std::string& LevelPath = Manager.GetCurrentLevelPath();
  return LevelPath.empty() ? std::string() : std::filesystem::path(LevelPath).stem().string();
}
}  // namespace

FAutomationStateProvider CreateAutomationStateProvider(
    const FAutomationRuntimeState& RuntimeState
) {
  return [&RuntimeState]() {
    nlohmann::json State = {
        {"sceneName", ""},
        {"fps", 0.0f},
        {"paused", RuntimeState.bPaused},
        {"worldAvailable", false},
        {"actorCount", 0u}
    };
    SceneManager& Manager = SceneManager::GetInstance();
    World* CurrentWorld = Manager.GetCurrentScene();
    if (CurrentWorld) {
      State["sceneName"] = GetCurrentSceneName(Manager);
      State["fps"] = CurrentWorld->GetCurrentFps();
      State["worldAvailable"] = true;
      if (const FActorManager* ActorManager = CurrentWorld->GetActorManager()) {
        State["actorCount"] = ActorManager->GetActiveActorCount();
      }
    }
    return State;
  };
}
