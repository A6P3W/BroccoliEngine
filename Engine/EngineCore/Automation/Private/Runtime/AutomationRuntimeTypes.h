#pragma once

#include <functional>
#include <nlohmann/json.hpp>

struct FAutomationRuntimeState {
  bool bPaused = false;
};

using FAutomationStateProvider = std::function<nlohmann::json()>;
