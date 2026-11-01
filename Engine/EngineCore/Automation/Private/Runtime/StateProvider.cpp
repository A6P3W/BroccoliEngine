#include "StateProvider.h"

#include "Application.h"

FAutomationStateProvider CreateAutomationStateProvider(
    FAutomationWorldStateProvider WorldStateProvider
) {
  return [WorldStateProvider = std::move(WorldStateProvider)]() {
    const FAutomationWorldStateSnapshot WorldState = WorldStateProvider();
    nlohmann::json State = {
        {"sceneName", WorldState.SceneName},
        {"fps", WorldState.Fps},
        {"simulating", WorldState.Simulating},
        {"worldAvailable", WorldState.WorldAvailable},
        {"actorCount", WorldState.ActorCount},
        {"physics3DAvailable", WorldState.Physics3DAvailable},
        {"physics3DBodyCount", WorldState.Physics3DBodyCount}
    };
    if (const auto* Control = Application::GetUpdateControl()) State.update(Control->GetState());
    return State;
  };
}
