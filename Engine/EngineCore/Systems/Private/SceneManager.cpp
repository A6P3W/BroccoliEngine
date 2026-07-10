#include "SceneManager.h"

#include "Actor.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include "GameModeBase.h"
#include "GridLine.h"
#include "LevelSerializer.h"
#include "ActorManager.h"
#include "RenderSystem.h"
#include "ReplicationSystem.h"

void SceneManager::RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath) {
  if (SceneId == 0 || LevelPath.empty()) {
    return;
  }
  RegisteredLevelPaths[SceneId] = LevelPath;
}

bool SceneManager::OpenLevelById(FNetworkSceneId SceneId) {
  return OpenLevelById(SceneId, GetCurrentNetMode());
}

bool SceneManager::OpenLevelById(FNetworkSceneId SceneId, ENetMode NetMode) {
  auto it = RegisteredLevelPaths.find(SceneId);
  if (it == RegisteredLevelPaths.end()) {
    return false;
  }
  QueueLevelPath(it->second, NetMode, SceneId);
  return true;
}

bool SceneManager::OpenLevelByPath(const std::string& LevelPath) {
  return OpenLevelByPath(LevelPath, GetCurrentNetMode());
}

bool SceneManager::OpenLevelByPath(const std::string& LevelPath, ENetMode NetMode) {
  if (LevelPath.empty()) {
    return false;
  }
  QueueLevelPath(LevelPath, NetMode, 0);
  return true;
}

bool SceneManager::OpenStartupLevel() {
  return OpenLevelByPath(StartupLevelPath, ENetMode::Standalone);
}

bool SceneManager::IsSceneRegistered(FNetworkSceneId SceneId) const {
  return RegisteredLevelPaths.find(SceneId) != RegisteredLevelPaths.end();
}

ENetMode SceneManager::GetCurrentNetMode() const {
  if (!CurrentScene) {
    return ENetMode::Standalone;
  }
  return CurrentScene->GetNetMode();
}

void SceneManager::QueueSceneFactory(
    RegisteredSceneFactory Factory, ENetMode NetMode, FNetworkSceneId SceneId
) {
  if (!Factory) {
    return;
  }
  PendingSceneFactory = [Factory = std::move(Factory), NetMode]() -> std::unique_ptr<World> {
    return Factory(NetMode);
  };
  PendingSceneId = SceneId;
  PendingLevelPath.clear();
}

void SceneManager::QueueLevelPath(
    const std::string& LevelPath, ENetMode NetMode, FNetworkSceneId SceneId
) {
  PendingSceneFactory = [LevelPath, NetMode]() -> std::unique_ptr<World> {
    auto newWorld = std::make_unique<World>();
    newWorld->SetNetMode(NetMode);
    if (!LevelSerializer::Load(newWorld.get(), LevelPath, true)) {
      return nullptr;
    }
    return newWorld;
  };
  PendingSceneId = SceneId;
  PendingLevelPath = LevelPath;
}

void SceneManager::ProcessSceneChanges() {
  if (PendingSceneFactory) {
    RenderSystem::GetInstance().SetCameraView(nullptr);
    auto sceneFactory = std::move(PendingSceneFactory);
    const FNetworkSceneId newSceneId = PendingSceneId;
    const std::string newLevelPath = PendingLevelPath;
    PendingSceneFactory = nullptr;
    PendingSceneId = 0;
    PendingLevelPath.clear();

    CurrentScene.reset();
    CurrentSceneId = 0;
    CurrentLevelPath.clear();

    auto newScene = sceneFactory();
    if (!newScene) {
      return;
    }

    newScene->GetObjectManager()->FlushPendingActors();

    CurrentScene = std::move(newScene);
    CurrentSceneId = newSceneId;
    CurrentLevelPath = newLevelPath;

    if (CurrentScene && IsDebug) {
      CurrentScene->SpawnActor<AGridLine>();
    }

    if (CurrentScene && CurrentScene->IsServer()) {
      if (AGameModeBase* gameMode = CurrentScene->GetGameMode()) {
        if (!gameMode->IsHostPlayerSpawned()) {
          gameMode->SpawnDefaultPlayer(0);
        }
      } else {
        CurrentScene->GetOrCreateLocalPlayerController();
      }
    }

    if (CurrentScene && CurrentScene->GetReplicationSystem()) {
      CurrentScene->GetReplicationSystem()->NotifySceneLoaded();
    }
    M_LOG(
        "Scene changed to ID: {}, NetMode: {}",
        CurrentSceneId,
        static_cast<int>(CurrentScene->GetNetMode())
    );
  }
}

void SceneManager::Shutdown() {
  RenderSystem::GetInstance().SetCameraView(nullptr);
  PendingSceneFactory = nullptr;
  PendingSceneId = 0;
  PendingLevelPath.clear();
  CurrentScene.reset();
  CurrentSceneId = 0;
  CurrentLevelPath.clear();
}
