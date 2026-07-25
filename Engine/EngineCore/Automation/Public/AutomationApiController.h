#pragma once

#include <functional>
#include <nlohmann/json.hpp>

#include "AutomationCommandQueue.h"
#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

struct FAutomationHttpResponse {
  int StatusCode = 500;
  nlohmann::json Body =
      MakeAutomationError(EAutomationErrorCode::InternalError, "The automation request failed.");
};

using FAutomationStateProvider = std::function<nlohmann::json()>;

class BROCCOLI_ENGINE_API FAutomationApiController {
 public:
  FAutomationApiController(
      FAutomationCommandQueue& InCommandQueue,
      const FAutomationConfig& InConfig,
      FAutomationStateProvider InStateProvider
  );

  FAutomationHttpResponse GetState();

 private:
  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
  FAutomationStateProvider StateProvider;
};
