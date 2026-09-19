#include "StateProvider.h"

FAutomationStateProvider CreateAutomationStateProvider(
    const FAutomationRuntimeState& RuntimeState, FAutomationWorldStateProvider WorldStateProvider
) {
  return [&RuntimeState, WorldStateProvider = std::move(WorldStateProvider)]() {
    const FAutomationWorldStateSnapshot WorldState = WorldStateProvider();
    nlohmann::json State = {
        {"sceneName", WorldState.SceneName},
        {"fps", WorldState.Fps},
        {"paused", RuntimeState.Paused},
        {"worldAvailable", WorldState.WorldAvailable},
        {"actorCount", WorldState.ActorCount},
        {"physics3DAvailable", WorldState.Physics3DAvailable},
        {"physics3DBodyCount", WorldState.Physics3DBodyCount}
    };
    return State;
  };
}
