#pragma once

class FAutomationSystemCommandRegistry;
struct FAutomationRuntimeState;

void RegisterAutomationBuiltInCommands(
    FAutomationSystemCommandRegistry& Registry, FAutomationRuntimeState& RuntimeState
);
