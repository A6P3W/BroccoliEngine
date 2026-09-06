#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

#include "AutomationTypes.h"
#include "Registry/ComponentMethodRegistry.h"
#include "Registry/ActorMethodRegistry.h"
#include "Registry/SystemCommandRegistry.h"
#include "Runtime/CommandQueue.h"
#include "Runtime/RuntimeState.h"
#include "Http/HttpTypes.h"
#include "World/DiscoveryTypes.h"
#include "World/WorldTypes.h"

class FAutomationHttpRequestExecutor {
 public:
  FAutomationHttpRequestExecutor(
      FAutomationCommandQueue& InCommandQueue, const FAutomationConfig& InConfig
  );

  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);

 private:
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
};

class FAutomationHttpControllerBase {
 protected:
  explicit FAutomationHttpControllerBase(FAutomationHttpRequestExecutor& InExecutor)
      : Executor(InExecutor) {}

  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket) {
    return Executor.WaitForResult(std::move(Ticket));
  }

  FAutomationHttpRequestExecutor& Executor;
};

