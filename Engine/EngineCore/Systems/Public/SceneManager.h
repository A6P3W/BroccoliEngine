#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "GameInstance.h"
#include "GameModeBase.h"
#include "NetMode.h"
#include "NetworkTypes.h"
#include "World.h"
class SceneManager {
 public:
  static SceneManager& GetInstance() {
    static SceneManager instance;
    return instance;
  }
  template <class T>
  void OpenScene() {
    QueueSceneFactory(MakeSceneFactory<T>(), GetCurrentNetMode(), 0);
  }

  World* GetCurrentScene() { return CurrentScene.get(); }

  void ProcessSceneChanges();

  void RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath);
  bool OpenLevelById(FNetworkSceneId SceneId);
  bool OpenLevelById(FNetworkSceneId SceneId, ENetMode NetMode);
  bool OpenLevelByPath(const std::string& LevelPath);
  bool OpenLevelByPath(const std::string& LevelPath, ENetMode NetMode);
  bool IsSceneRegistered(FNetworkSceneId SceneId) const;
  FNetworkSceneId GetCurrentSceneId() const { return CurrentSceneId; }
  const std::string& GetCurrentLevelPath() const { return CurrentLevelPath; }
  ENetMode GetCurrentNetMode() const;

  template <class T>
  void SetGameInstance() {
    GameInstance = std::make_unique<T>();
  }
  GameInstance* GetGameInstance() { return GameInstance.get(); }

 private:
  using RegisteredSceneFactory = std::function<std::unique_ptr<World>(ENetMode)>;

  template <class T>
  static RegisteredSceneFactory MakeSceneFactory() {
    return [](ENetMode NetMode) -> std::unique_ptr<World> {
      auto newWorld = std::make_unique<World>();
      newWorld->SetNetMode(NetMode);
      if (newWorld->IsServer()) {
        newWorld->SpawnGameMode<T>();
      }
      return newWorld;
    };
  }

  void QueueSceneFactory(RegisteredSceneFactory Factory, ENetMode NetMode, FNetworkSceneId SceneId);
  void QueueLevelPath(const std::string& LevelPath, ENetMode NetMode, FNetworkSceneId SceneId);

  std::unique_ptr<World> CurrentScene;
  std::function<std::unique_ptr<World>()> PendingSceneFactory;
  std::unordered_map<FNetworkSceneId, std::string> RegisteredLevelPaths;
  std::unique_ptr<GameInstance> GameInstance = nullptr;
  FNetworkSceneId CurrentSceneId = 0;
  FNetworkSceneId PendingSceneId = 0;
  std::string CurrentLevelPath;
  std::string PendingLevelPath;
};
