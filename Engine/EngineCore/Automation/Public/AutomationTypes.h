#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#include "AutomationError.h"

struct FAutomationConfig {
  bool Enabled = false;
  std::string BindAddress = "127.0.0.1";
  uint16_t Port = 39100;
  uint32_t RequestTimeoutSeconds = 3;
  size_t MaxRequestBodyBytes = 1024 * 1024;
};

nlohmann::json MakeAutomationSuccess(nlohmann::json Data = nlohmann::json::object());

bool IsValidAutomationOperationName(std::string_view Name);
