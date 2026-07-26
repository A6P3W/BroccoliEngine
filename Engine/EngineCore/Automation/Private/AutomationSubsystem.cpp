#include "AutomationSubsystem.h"

#include <filesystem>
#include "ActorManager.h"
#include "AutomationApiController.h"
#include "AutomationAutoRegistrar.h"
#include "AutomationCommandQueue.h"
#include "AutomationDiscoveryAdapter.h"
#include "AutomationHttpServer.h"
#include "AutomationRegistryHelper.h"
#include "AutomationSystemCommandRegistry.h"
#include "AutomationWorldAdapter.h"
#include "Log.h"
#include "NetworkTypes.h"
#include "SceneManager.h"
#include "World.h"

namespace {
std::string GetCurrentSceneName(const SceneManager& Manager) {
  const std::string& LevelPath = Manager.GetCurrentLevelPath();
  return LevelPath.empty() ? std::string() : std::filesystem::path(LevelPath).stem().string();
}
}  // namespace

FAutomationSubsystem::FAutomationSubsystem() = default;

FAutomationSubsystem::~FAutomationSubsystem() { Shutdown(); }

bool FAutomationSubsystem::Initialize(const FAutomationConfig& Config) {
  Shutdown();
  if (!Config.Enabled) {
    M_LOG("Automation disabled.");
    return true;
  }

  CommandQueue = std::make_unique<FAutomationCommandQueue>();
  MethodRegistry = std::make_unique<FAutomationMethodRegistry>();
  try {
    FAutomationAutoRegistrar::GetInstance().RegisterAll(*MethodRegistry);
    MethodRegistry->Freeze();
  } catch (const std::exception& Exception) {
    M_LOG("Automation actor method registration failed: {}", Exception.what());
    Shutdown();
    return false;
  } catch (...) {
    M_LOG("Automation actor method registration failed with an unknown exception.");
    Shutdown();
    return false;
  }

  SystemCommandRegistry = std::make_unique<FAutomationSystemCommandRegistry>();
  try {
    RegisterBuiltInSystemCommands();
    SystemCommandRegistry->Freeze();
  } catch (const std::exception& Exception) {
    M_LOG("Automation system command registration failed: {}", Exception.what());
    Shutdown();
    return false;
  } catch (...) {
    M_LOG("Automation system command registration failed with an unknown exception.");
    Shutdown();
    return false;
  }

  FAutomationStateProvider StateProvider = [this]() {
    nlohmann::json State = {
        {"sceneName", ""},
        {"fps", 0.0f},
        {"paused", bPaused},
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

  WorldAdapter = std::make_unique<FAutomationWorldAdapter>();
  FAutomationDiscoveryAdapter DiscoveryAdapter;
  ApiController = std::make_unique<FAutomationApiController>(
      *CommandQueue,
      Config,
      std::move(StateProvider),
      WorldAdapter->CreateActorListProvider(),
      WorldAdapter->CreateActorProvider(),
      WorldAdapter->CreateSpawnActorProvider(),
      WorldAdapter->CreateDestroyActorProvider(),
      WorldAdapter->CreateTransformProvider(),
      MethodRegistry.get(),
      WorldAdapter->CreateActorResolver(),
      SystemCommandRegistry.get(),
      DiscoveryAdapter.CreateActorClassListProvider(),
      DiscoveryAdapter.CreateLevelListProvider(),
      DiscoveryAdapter.CreateActorClassExistsProvider()
  );
  HttpServer = std::make_unique<FAutomationHttpServer>(Config, *ApiController);
  if (!HttpServer->Start()) {
    M_LOG("Automation server startup failed; the engine will continue without Automation.");
    Shutdown();
    return false;
  }
  return true;
}

void FAutomationSubsystem::RegisterBuiltInSystemCommands() {
  AutomationHelper::RegisterSystemCommand(
      *SystemCommandRegistry,
      "pause_game",
      "Pause world updates while keeping automation available.",
      [this]() {
        const bool Changed = !bPaused;
        bPaused = true;
        M_LOG(
            "Automation system command state changed: command=pause_game changed={} paused=true",
            Changed
        );
        return nlohmann::json{
            {"commandName", "pause_game"}, {"changed", Changed}, {"paused", true}};
      }
  );
  AutomationHelper::RegisterSystemCommand(
      *SystemCommandRegistry, "resume_game", "Resume world updates.", [this]() {
        const bool Changed = bPaused;
        bPaused = false;
        M_LOG(
            "Automation system command state changed: command=resume_game changed={} paused=false",
            Changed
        );
        return nlohmann::json{
            {"commandName", "resume_game"}, {"changed", Changed}, {"paused", false}};
      }
  );
  AutomationHelper::RegisterSystemCommand(
      *SystemCommandRegistry,
      "open_level_by_id",
      "Queue a registered level to open by scene ID.",
      [](FNetworkSceneId SceneId) {
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(SceneId)
                                         : Manager.OpenLevelById(SceneId, ENetMode::Standalone);
        return nlohmann::json{
            {"commandName", "open_level_by_id"}, {"sceneId", SceneId}, {"queued", Queued}};
      },
      AutomationParam<FNetworkSceneId>("sceneId", "Registered scene ID.")
  );
  AutomationHelper::RegisterSystemCommand(
      *SystemCommandRegistry,
      "open_level_by_path",
      "Queue a level to open by file path.",
      [](std::string LevelPath) {
        SceneManager& Manager = SceneManager::GetInstance();
        World* CurrentWorld = Manager.GetCurrentScene();
        const bool Queued = CurrentWorld ? CurrentWorld->ServerTravel(LevelPath)
                                         : Manager.OpenLevelByPath(LevelPath, ENetMode::Standalone);
        return nlohmann::json{
            {"commandName", "open_level_by_path"}, {"levelPath", LevelPath}, {"queued", Queued}};
      },
      AutomationParam<std::string>("levelPath", "Level file path.")
  );
}

void FAutomationSubsystem::Update() {
  if (CommandQueue) {
    CommandQueue->ProcessCommands();
  }
}

void FAutomationSubsystem::Shutdown() {
  if (HttpServer) {
    HttpServer->StopAcceptingRequests();
  }
  if (CommandQueue) {
    CommandQueue->StopAcceptingCommands();
    CommandQueue->CancelAll(
        EAutomationErrorCode::EngineShuttingDown, "The engine is shutting down."
    );
  }
  if (HttpServer) {
    HttpServer->Stop();
  }

  HttpServer.reset();
  ApiController.reset();
  WorldAdapter.reset();
  SystemCommandRegistry.reset();
  MethodRegistry.reset();
  CommandQueue.reset();
  bPaused = false;
}

bool FAutomationSubsystem::IsRunning() const { return HttpServer && HttpServer->IsRunning(); }

bool FAutomationSubsystem::IsPaused() const { return bPaused; }
