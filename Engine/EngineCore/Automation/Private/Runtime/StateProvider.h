#pragma once

#include <functional>
#include <nlohmann/json.hpp>

#include "World/WorldTypes.h"

using FAutomationStateProvider = std::function<nlohmann::json()>;

FAutomationStateProvider CreateAutomationStateProvider(
    FAutomationWorldStateProvider WorldStateProvider
);
