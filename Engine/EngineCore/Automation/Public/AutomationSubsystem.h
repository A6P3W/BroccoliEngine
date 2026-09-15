#pragma once

#include <memory>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

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
  struct FImpl;
  std::unique_ptr<FImpl> ImplPtr;
};
