#include "AutomationSubsystem.h"

#include <exception>
#include <memory>
#include <utility>

#include "AutomationAutoRegistrar.h"
#include "AutomationComponentMethodRegistry.h"
#include "AutomationMethodRegistry.h"
#include "AutomationSystemCommandRegistry.h"
#include "Log.h"
#include "Runtime/AutomationBuiltInCommands.h"
#include "Runtime/AutomationCommandQueue.h"
#include "Runtime/AutomationRuntimeTypes.h"
#include "Runtime/AutomationStateProvider.h"
#include "Transport/Http/AutomationApiController.h"
#include "Transport/Http/AutomationHttpServer.h"
#include "World/AutomationDiscoveryAdapter.h"
#include "World/AutomationWorldAdapter.h"

struct FAutomationSubsystem::FImpl {
  bool Initialize(const FAutomationConfig& Config) {
    Shutdown();
    if (!Config.Enabled) {
      M_LOG(Log, "Automation disabled.");
      return true;
    }

    if (!CreateRegistries()) {
      Shutdown();
      return false;
    }

    CommandQueue = std::make_unique<FAutomationCommandQueue>();
    WorldAdapter = std::make_unique<FAutomationWorldAdapter>();
    FAutomationDiscoveryAdapter DiscoveryAdapter;
    ApiController = std::make_unique<FAutomationApiController>(
        *CommandQueue,
        Config,
        CreateAutomationStateProvider(RuntimeState),
        WorldAdapter->CreateActorListProvider(),
        WorldAdapter->CreateActorProvider(),
        WorldAdapter->CreateActorComponentListProvider(),
        WorldAdapter->CreateSpawnActorProvider(),
        WorldAdapter->CreateDestroyActorProvider(),
        WorldAdapter->CreateTransformProvider(),
        MethodRegistry.get(),
        WorldAdapter->CreateActorResolver(),
        ComponentMethodRegistry.get(),
        WorldAdapter->CreateComponentResolver(),
        SystemCommandRegistry.get(),
        DiscoveryAdapter.CreateActorClassListProvider(),
        DiscoveryAdapter.CreateLevelListProvider(),
        DiscoveryAdapter.CreateActorClassExistsProvider()
    );
    HttpServer = std::make_unique<FAutomationHttpServer>(Config, *ApiController);
    if (!HttpServer->Start()) {
      M_LOG(Log, "Automation server startup failed; the engine will continue without Automation.");
      Shutdown();
      return false;
    }
    return true;
  }

  bool CreateRegistries() {
    MethodRegistry = std::make_unique<FAutomationMethodRegistry>();
    ComponentMethodRegistry = std::make_unique<FAutomationComponentMethodRegistry>();
    try {
      FAutomationAutoRegistrar::GetInstance().RegisterAll(*MethodRegistry);
      FAutomationAutoRegistrar::GetInstance().RegisterAllComponents(*ComponentMethodRegistry);
      FAutomationAutoRegistrar::GetInstance().RegisterAllUnified(
          *MethodRegistry, *ComponentMethodRegistry
      );
      MethodRegistry->Freeze();
      ComponentMethodRegistry->Freeze();
    } catch (const std::exception& Exception) {
      M_LOG(Log, "Automation method registration failed: {}", Exception.what());
      return false;
    } catch (...) {
      M_LOG(Log, "Automation method registration failed with an unknown exception.");
      return false;
    }

    SystemCommandRegistry = std::make_unique<FAutomationSystemCommandRegistry>();
    try {
      RegisterAutomationBuiltInCommands(*SystemCommandRegistry, RuntimeState);
      SystemCommandRegistry->Freeze();
    } catch (const std::exception& Exception) {
      M_LOG(Log, "Automation system command registration failed: {}", Exception.what());
      return false;
    } catch (...) {
      M_LOG(Log, "Automation system command registration failed with an unknown exception.");
      return false;
    }
    return true;
  }

  void Update() {
    if (CommandQueue) {
      CommandQueue->ProcessCommands();
    }
  }

  void Shutdown() {
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
    ComponentMethodRegistry.reset();
    MethodRegistry.reset();
    CommandQueue.reset();
    RuntimeState.bPaused = false;
  }

  FAutomationRuntimeState RuntimeState;
  std::unique_ptr<FAutomationCommandQueue> CommandQueue;
  std::unique_ptr<FAutomationMethodRegistry> MethodRegistry;
  std::unique_ptr<FAutomationComponentMethodRegistry> ComponentMethodRegistry;
  std::unique_ptr<FAutomationSystemCommandRegistry> SystemCommandRegistry;
  std::unique_ptr<FAutomationWorldAdapter> WorldAdapter;
  std::unique_ptr<FAutomationApiController> ApiController;
  std::unique_ptr<FAutomationHttpServer> HttpServer;
};

FAutomationSubsystem::FAutomationSubsystem() : ImplPtr(std::make_unique<FImpl>()) {}

FAutomationSubsystem::~FAutomationSubsystem() { Shutdown(); }

bool FAutomationSubsystem::Initialize(const FAutomationConfig& Config) {
  return ImplPtr->Initialize(Config);
}

void FAutomationSubsystem::Update() { ImplPtr->Update(); }

void FAutomationSubsystem::Shutdown() {
  if (ImplPtr) {
    ImplPtr->Shutdown();
  }
}

bool FAutomationSubsystem::IsRunning() const {
  return ImplPtr && ImplPtr->HttpServer && ImplPtr->HttpServer->IsRunning();
}

bool FAutomationSubsystem::IsPaused() const { return ImplPtr && ImplPtr->RuntimeState.bPaused; }
