#pragma once
#include "BroccoliEngineAPI.h"
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
class BROCCOLI_ENGINE_API SceneManager {
 public:
  static SceneManager& GetInstance();
  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;
  template <class T>
  void OpenGameMode() {
    QueueSceneFactory(MakeSceneFactory<T>(), GetCurrentNetMode(), 0);
  }

  World* GetCurrentScene();

  void ProcessSceneChanges();
  void Shutdown();

  void RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath);
  bool OpenLevelById(FNetworkSceneId SceneId);
  bool OpenLevelById(FNetworkSceneId SceneId, ENetMode NetMode);
  bool OpenLevelByPath(const std::string& LevelPath);
  bool OpenLevelByPath(const std::string& LevelPath, ENetMode NetMode);
  bool IsSceneRegistered(FNetworkSceneId SceneId) const;
  FNetworkSceneId GetCurrentSceneId() const;
  const std::string& GetCurrentLevelPath() const;
  ENetMode GetCurrentNetMode() const;
  void SetStartupLevelPath(const std::string& LevelPath);
  const std::string& GetStartupLevelPath() const;
  bool OpenStartupLevel();

  template <class T>
  void SetGameInstance() {
    SetGameInstanceInternal(std::make_unique<T>());
  }
  GameInstance* GetGameInstance();

 private:
  SceneManager();
  ~SceneManager();
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
  void SetGameInstanceInternal(std::unique_ptr<GameInstance> NewGameInstance);
  struct Impl;
  Impl* ImplPtr = nullptr;
};
