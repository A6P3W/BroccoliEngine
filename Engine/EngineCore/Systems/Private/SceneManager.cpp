#include "SceneManager.h"

#include "Actor.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include "GameModeBase.h"
#include "GridLine.h"
#include "LevelSerializer.h"
#include "ObjectManager.h"
#include "RenderSystem.h"
#include "ReplicationSystem.h"

void SceneManager::RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath) {
  if (SceneId == 0 || LevelPath.empty()) {
    return;
  }
  RegisteredLevelPaths[SceneId] = LevelPath;
}

bool SceneManager::OpenSceneById(FNetworkSceneId SceneId) {
  return OpenSceneById(SceneId, GetCurrentNetMode());
}

bool SceneManager::OpenSceneById(FNetworkSceneId SceneId, ENetMode NetMode) {
  auto it = RegisteredLevelPaths.find(SceneId);
  if (it == RegisteredLevelPaths.end()) {
    return false;
  }
  QueueLevelPath(it->second, NetMode, SceneId);
  return true;
}

bool SceneManager::OpenLevel(const std::string& LevelPath) {
  return OpenLevel(LevelPath, GetCurrentNetMode());
}

bool SceneManager::OpenLevel(const std::string& LevelPath, ENetMode NetMode) {
  if (LevelPath.empty()) {
    return false;
  }
  QueueLevelPath(LevelPath, NetMode, 0);
  return true;
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
}

void SceneManager::ProcessSceneChanges() {
  if (PendingSceneFactory) {
    RenderSystem::GetInstance().SetCameraView(nullptr);
    auto sceneFactory = std::move(PendingSceneFactory);
    const FNetworkSceneId newSceneId = PendingSceneId;
    PendingSceneFactory = nullptr;
    PendingSceneId = 0;

    CurrentScene.reset();
    CurrentSceneId = 0;

    auto newScene = sceneFactory();
    if (!newScene) {
      return;
    }
    CurrentScene = std::move(newScene);
    CurrentSceneId = newSceneId;

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
