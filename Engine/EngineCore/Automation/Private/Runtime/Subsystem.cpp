#include "AutomationSubsystem.h"

#include <exception>
#include <memory>
#include <utility>

#include "Log.h"
#include "Registration/RegistrationStore.h"
#include "Registry/ComponentMethodRegistry.h"
#include "Registry/ActorMethodRegistry.h"
#include "Registry/SystemCommandRegistry.h"
#include "Runtime/BuiltInCommands.h"
#include "Runtime/CommandQueue.h"
#include "Runtime/RuntimeState.h"
#include "Runtime/StateProvider.h"
#include "Transport/Http/AutomationHttpControllers.h"
#include "Transport/Http/AutomationHttpServer.h"
#include "World/DiscoveryService.h"
#include "World/WorldService.h"

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
    WorldAdapter = std::make_unique<FAutomationWorldService>();
    FAutomationDiscoveryService DiscoveryAdapter;
    HttpRequestExecutor = std::make_unique<FAutomationHttpRequestExecutor>(*CommandQueue, Config);
    WorldController = std::make_unique<FAutomationWorldController>(
        *HttpRequestExecutor,
        *CommandQueue,
        CreateAutomationStateProvider(RuntimeState),
        WorldAdapter->CreateActorListProvider(),
        WorldAdapter->CreateActorProvider(),
        WorldAdapter->CreateActorComponentListProvider(),
        WorldAdapter->CreateSpawnActorProvider(),
        WorldAdapter->CreateDestroyActorProvider(),
        WorldAdapter->CreateTransformProvider()
    );
    DiscoveryController = std::make_unique<FAutomationDiscoveryController>(
        *HttpRequestExecutor,
        *CommandQueue,
        *MethodRegistry,
        DiscoveryAdapter.CreateActorClassListProvider(),
        DiscoveryAdapter.CreateLevelListProvider(),
        DiscoveryAdapter.CreateActorClassExistsProvider()
    );
    InvocationController = std::make_unique<FAutomationInvocationController>(
        *HttpRequestExecutor,
        *CommandQueue,
        *MethodRegistry,
        WorldAdapter->CreateActorResolver(),
        *ComponentMethodRegistry,
        WorldAdapter->CreateComponentResolver()
    );
    SystemController = std::make_unique<FAutomationSystemController>(
        *HttpRequestExecutor, *CommandQueue, *SystemCommandRegistry
    );
    LogController = std::make_unique<FAutomationLogController>(*HttpRequestExecutor, *CommandQueue);
    HttpServer = std::make_unique<FAutomationHttpServer>(
        Config,
        FAutomationHttpControllers{
            *WorldController,
            *DiscoveryController,
            *InvocationController,
            *SystemController,
            *LogController
        }
    );
    if (!HttpServer->Start()) {
      M_LOG(Log, "Automation server startup failed; the engine will continue without Automation.");
      Shutdown();
      return false;
    }
    return true;
  }

  bool CreateRegistries() {
    MethodRegistry = std::make_unique<FAutomationActorMethodRegistry>();
    ComponentMethodRegistry = std::make_unique<FAutomationComponentMethodRegistry>();
    try {
      RegisterAllAutomationMethods(*MethodRegistry, *ComponentMethodRegistry);
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
    LogController.reset();
    SystemController.reset();
    InvocationController.reset();
    DiscoveryController.reset();
    WorldController.reset();
    HttpRequestExecutor.reset();
    WorldAdapter.reset();
    SystemCommandRegistry.reset();
    ComponentMethodRegistry.reset();
    MethodRegistry.reset();
    CommandQueue.reset();
    RuntimeState.bPaused = false;
  }

  FAutomationRuntimeState RuntimeState;
  std::unique_ptr<FAutomationCommandQueue> CommandQueue;
  std::unique_ptr<FAutomationActorMethodRegistry> MethodRegistry;
  std::unique_ptr<FAutomationComponentMethodRegistry> ComponentMethodRegistry;
  std::unique_ptr<FAutomationSystemCommandRegistry> SystemCommandRegistry;
  std::unique_ptr<FAutomationWorldService> WorldAdapter;
  std::unique_ptr<FAutomationHttpRequestExecutor> HttpRequestExecutor;
  std::unique_ptr<FAutomationWorldController> WorldController;
  std::unique_ptr<FAutomationDiscoveryController> DiscoveryController;
  std::unique_ptr<FAutomationInvocationController> InvocationController;
  std::unique_ptr<FAutomationSystemController> SystemController;
  std::unique_ptr<FAutomationLogController> LogController;
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
