#include "AutomationDiscoveryAdapter.h"

#include <algorithm>
#include "ActorRegistry.h"
#include "SceneManager.h"

FAutomationActorClassListProvider FAutomationDiscoveryAdapter::CreateActorClassListProvider() {
  return []() {
    const ActorRegistry& Registry = ActorRegistry::GetInstance();
    const std::vector<std::string>& GameModeClassNames = Registry.GetGameModeClassNames();
    std::vector<FAutomationActorClassInfo> Result;
    const std::vector<std::string>& ClassNames = Registry.GetClassNames();
    Result.reserve(ClassNames.size() + GameModeClassNames.size());
    for (const std::string& ClassName : ClassNames) {
      Result.push_back({ClassName, false});
    }
    for (const std::string& ClassName : GameModeClassNames) {
      Result.push_back({ClassName, true});
    }
    std::sort(
        Result.begin(),
        Result.end(),
        [](const FAutomationActorClassInfo& Left, const FAutomationActorClassInfo& Right) {
          return Left.ClassName < Right.ClassName;
        }
    );
    return Result;
  };
}

FAutomationLevelListProvider FAutomationDiscoveryAdapter::CreateLevelListProvider() {
  return []() {
    std::vector<FAutomationLevelInfo> Result;
    for (const SceneManager::FRegisteredLevelSnapshot& Snapshot :
         SceneManager::GetInstance().GetRegisteredLevels()) {
      Result.push_back({Snapshot.SceneId, Snapshot.LevelPath});
    }
    return Result;
  };
}

FAutomationActorClassExistsProvider FAutomationDiscoveryAdapter::CreateActorClassExistsProvider() {
  return [](std::string_view ClassName) {
    return ActorRegistry::GetInstance().Contains(std::string(ClassName));
  };
}
