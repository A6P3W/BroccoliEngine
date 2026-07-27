#pragma once

#include <memory>

#include "AutomationMethodRegistry.h"
#include "AutomationComponentMethodRegistry.h"
#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

class FAutomationApiController;
class FAutomationCommandQueue;
class FAutomationHttpServer;
class FAutomationSystemCommandRegistry;
class FAutomationWorldAdapter;

class BROCCOLI_ENGINE_API FAutomationSubsystem {
 public:
  FAutomationSubsystem();
  ~FAutomationSubsystem();

  FAutomationSubsystem(const FAutomationSubsystem&) = delete;
  FAutomationSubsystem& operator=(const FAutomationSubsystem&) = delete;

  bool Initialize(const FAutomationConfig& Config);
  void Update();
  void Shutdown();

  bool IsRunning() const;
 bool IsPaused() const;

 private:
  void RegisterBuiltInSystemCommands();

  bool bPaused = false;
  std::unique_ptr<FAutomationCommandQueue> CommandQueue;
  std::unique_ptr<FAutomationMethodRegistry> MethodRegistry;
  std::unique_ptr<FAutomationComponentMethodRegistry> ComponentMethodRegistry;
  std::unique_ptr<FAutomationSystemCommandRegistry> SystemCommandRegistry;
  std::unique_ptr<FAutomationWorldAdapter> WorldAdapter;
  std::unique_ptr<FAutomationApiController> ApiController;
  std::unique_ptr<FAutomationHttpServer> HttpServer;
};
