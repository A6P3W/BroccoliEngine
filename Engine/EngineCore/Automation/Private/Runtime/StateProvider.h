#pragma once

#include "Runtime/RuntimeState.h"
#include "World/WorldTypes.h"

FAutomationStateProvider CreateAutomationStateProvider(
    const FAutomationRuntimeState& RuntimeState, FAutomationWorldStateProvider WorldStateProvider
);
