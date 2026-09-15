#pragma once

#include <functional>
#include <nlohmann/json.hpp>

struct FAutomationRuntimeState {
  bool Paused = false;
};

using FAutomationStateProvider = std::function<nlohmann::json()>;
