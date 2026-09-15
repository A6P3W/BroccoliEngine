#pragma once

#include "HttpControllerBase.h"

#include <nlohmann/json.hpp>
#include <string_view>

#include "Registry/SystemCommandRegistry.h"

class FAutomationSystemController final : public FAutomationHttpControllerBase {
 public:
  FAutomationSystemController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationSystemCommandRegistry& InSystemCommandRegistry
  );

  FAutomationHttpResponse GetSystemCommands();
  FAutomationHttpResponse ExecuteSystemCommand(
      std::string_view CommandName, const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationSystemCommandRegistry* SystemCommandRegistry = nullptr;
};
