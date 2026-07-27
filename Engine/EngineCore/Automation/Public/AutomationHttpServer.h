#pragma once

#include <memory>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

class FAutomationApiController;

class BROCCOLI_ENGINE_API FAutomationHttpServer {
 public:
  FAutomationHttpServer(
      const FAutomationConfig& InConfig, FAutomationApiController& InApiController
  );
  ~FAutomationHttpServer();

  FAutomationHttpServer(const FAutomationHttpServer&) = delete;
  FAutomationHttpServer& operator=(const FAutomationHttpServer&) = delete;

  bool Start();
  void StopAcceptingRequests();
  void Stop();
  bool IsRunning() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> ImplPtr;
};
