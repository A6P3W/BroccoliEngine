#pragma once

#include <nlohmann/json.hpp>

#include "AutomationTypes.h"
#include "Http/HttpTypes.h"
#include "Runtime/CommandQueue.h"

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
