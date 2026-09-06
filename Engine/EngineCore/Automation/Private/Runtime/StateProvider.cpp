#include "StateProvider.h"

FAutomationStateProvider CreateAutomationStateProvider(
    const FAutomationRuntimeState& RuntimeState, FAutomationWorldStateProvider WorldStateProvider
) {
  return [&RuntimeState, WorldStateProvider = std::move(WorldStateProvider)]() {
    const FAutomationWorldStateSnapshot WorldState = WorldStateProvider();
    nlohmann::json State = {
        {"sceneName", WorldState.SceneName},
        {"fps", WorldState.Fps},
        {"paused", RuntimeState.bPaused},
        {"worldAvailable", WorldState.bWorldAvailable},
        {"actorCount", WorldState.ActorCount}
    };
    return State;
  };
}
