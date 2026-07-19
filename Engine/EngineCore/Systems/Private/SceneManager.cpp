#include "SceneManager.h"

#include <filesystem>
#include <system_error>

namespace {
std::string NormalizeRuntimeLevelPath(const std::string& LevelPath) {
  const std::filesystem::path InputPath = std::filesystem::path(LevelPath).lexically_normal();
  if (!InputPath.is_absolute()) {
    return InputPath.generic_string();
  }

  std::error_code ErrorCode;
  const std::filesystem::path CurrentPath = std::filesystem::current_path(ErrorCode);
  if (ErrorCode) {
    return InputPath.string();
  }

  const std::filesystem::path RelativePath =
      std::filesystem::relative(InputPath, CurrentPath, ErrorCode);
  if (ErrorCode || RelativePath.empty()) {
    return InputPath.string();
  }

  const auto FirstComponent = RelativePath.begin();
  if (FirstComponent != RelativePath.end() && *FirstComponent != "..") {
    return RelativePath.generic_string();
  }
  return InputPath.string();
}
}  // namespace

struct SceneManager::Impl {
  std::unique_ptr<World> CurrentScene;
  std::function<std::unique_ptr<World>()> PendingSceneFactory;
  std::unordered_map<FNetworkSceneId, std::string> RegisteredLevelPaths;
  std::unique_ptr<GameInstance> GameInstance = std::make_unique<::GameInstance>();
  FNetworkSceneId CurrentSceneId = 0;
  FNetworkSceneId PendingSceneId = 0;
  std::string CurrentLevelPath;
  std::string PendingLevelPath;
  std::string StartupLevelPath;
};

SceneManager::SceneManager() : ImplPtr(new Impl()) {}

SceneManager::~SceneManager() { delete ImplPtr; }
SceneManager& SceneManager::GetInstance() {
  static SceneManager Instance;
  return Instance;
}

#include "Actor.h"
#include "ActorManager.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include "GameModeBase.h"
#include "GridLine.h"
#include "LevelSerializer.h"
#include "RenderSystem.h"
#include "ReplicationSystem.h"

World* SceneManager::GetCurrentScene() { return ImplPtr->CurrentScene.get(); }

FNetworkSceneId SceneManager::GetCurrentSceneId() const { return ImplPtr->CurrentSceneId; }

const std::string& SceneManager::GetCurrentLevelPath() const { return ImplPtr->CurrentLevelPath; }

void SceneManager::SetStartupLevelPath(const std::string& LevelPath) {
  ImplPtr->StartupLevelPath = LevelPath;
}

const std::string& SceneManager::GetStartupLevelPath() const { return ImplPtr->StartupLevelPath; }

GameInstance* SceneManager::GetGameInstance() { return ImplPtr->GameInstance.get(); }

void SceneManager::SetGameInstanceInternal(std::unique_ptr<GameInstance> NewGameInstance) {
  ImplPtr->GameInstance = std::move(NewGameInstance);
}
void SceneManager::RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath) {
  if (SceneId == 0 || LevelPath.empty()) {
    return;
  }
  ImplPtr->RegisteredLevelPaths[SceneId] = LevelPath;
}

bool SceneManager::OpenLevelById(FNetworkSceneId SceneId) {
  return OpenLevelById(SceneId, GetCurrentNetMode());
}

bool SceneManager::OpenLevelById(FNetworkSceneId SceneId, ENetMode NetMode) {
  auto it = ImplPtr->RegisteredLevelPaths.find(SceneId);
  if (it == ImplPtr->RegisteredLevelPaths.end()) {
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
  const std::string RuntimeLevelPath = NormalizeRuntimeLevelPath(LevelPath);
  M_LOG(
      "[SceneManager] Queue level: requested={} runtime={} netMode={}",
      LevelPath,
      RuntimeLevelPath,
      static_cast<int>(NetMode)
  );
  QueueLevelPath(RuntimeLevelPath, NetMode, 0);
  return true;
}

bool SceneManager::OpenStartupLevel() {
  return OpenLevelByPath(ImplPtr->StartupLevelPath, ENetMode::Standalone);
}

bool SceneManager::IsSceneRegistered(FNetworkSceneId SceneId) const {
  return ImplPtr->RegisteredLevelPaths.find(SceneId) != ImplPtr->RegisteredLevelPaths.end();
}

ENetMode SceneManager::GetCurrentNetMode() const {
  if (!ImplPtr->CurrentScene) {
    return ENetMode::Standalone;
  }
  return ImplPtr->CurrentScene->GetNetMode();
}

void SceneManager::QueueSceneFactory(
    RegisteredSceneFactory Factory, ENetMode NetMode, FNetworkSceneId SceneId
) {
  if (!Factory) {
    return;
  }
  ImplPtr->PendingSceneFactory = [Factory = std::move(Factory),
                                  NetMode]() -> std::unique_ptr<World> { return Factory(NetMode); };
  ImplPtr->PendingSceneId = SceneId;
  ImplPtr->PendingLevelPath.clear();
}

void SceneManager::QueueLevelPath(
    const std::string& LevelPath, ENetMode NetMode, FNetworkSceneId SceneId
) {
  ImplPtr->PendingSceneFactory = [LevelPath, NetMode]() -> std::unique_ptr<World> {
    auto newWorld = std::make_unique<World>();
    newWorld->SetNetMode(NetMode);
    if (!LevelSerializer::Load(newWorld.get(), LevelPath, true)) {
      return nullptr;
    }
    return newWorld;
  };
  ImplPtr->PendingSceneId = SceneId;
  ImplPtr->PendingLevelPath = LevelPath;
}

void SceneManager::ProcessSceneChanges() {
  if (ImplPtr->PendingSceneFactory) {
    auto SceneFactory = std::move(ImplPtr->PendingSceneFactory);
    const FNetworkSceneId NewSceneId = ImplPtr->PendingSceneId;
    const std::string NewLevelPath = ImplPtr->PendingLevelPath;
    ImplPtr->PendingSceneFactory = nullptr;
    ImplPtr->PendingSceneId = 0;
    ImplPtr->PendingLevelPath.clear();

    auto NewScene = SceneFactory();
    if (!NewScene) {
      M_LOG(
          "[SceneManager] Scene change failed; current scene preserved: id={} levelPath={}",
          NewSceneId,
          NewLevelPath
      );
      return;
    }

    ImplPtr->CurrentScene.reset();
    ImplPtr->CurrentSceneId = 0;
    ImplPtr->CurrentLevelPath.clear();

    NewScene->GetActorManager()->FlushPendingActors();

    ImplPtr->CurrentScene = std::move(NewScene);
    ImplPtr->CurrentSceneId = NewSceneId;
    ImplPtr->CurrentLevelPath = NewLevelPath;

    if (ImplPtr->CurrentScene && IsDebug) {
      ImplPtr->CurrentScene->SpawnActor<AGridLine>();
    }

    if (ImplPtr->CurrentScene && ImplPtr->CurrentScene->IsServer()) {
      if (AGameModeBase* gameMode = ImplPtr->CurrentScene->GetGameMode()) {
        if (!gameMode->IsHostPlayerSpawned()) {
          gameMode->SpawnDefaultPlayer(0);
        }
      } else {
        ImplPtr->CurrentScene->GetOrCreateLocalPlayerController();
      }
    }

    if (ImplPtr->CurrentScene && ImplPtr->CurrentScene->GetReplicationSystem()) {
      ImplPtr->CurrentScene->GetReplicationSystem()->NotifySceneLoaded();
    }
    M_LOG(
        "Scene changed to ID: {}, NetMode: {}, CameraActive: {}",
        ImplPtr->CurrentSceneId,
        static_cast<int>(ImplPtr->CurrentScene->GetNetMode()),
        RenderSystem::GetInstance().GetCamera() != nullptr
    );
  }
}

void SceneManager::Shutdown() {
  RenderSystem::GetInstance().SetCameraView(nullptr);
  ImplPtr->PendingSceneFactory = nullptr;
  ImplPtr->PendingSceneId = 0;
  ImplPtr->PendingLevelPath.clear();
  ImplPtr->CurrentScene.reset();
  ImplPtr->CurrentSceneId = 0;
  ImplPtr->CurrentLevelPath.clear();
}
