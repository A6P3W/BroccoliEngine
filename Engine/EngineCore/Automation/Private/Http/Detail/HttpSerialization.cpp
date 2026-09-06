#include "HttpSerialization.h"
#include <cmath>
#include <stdexcept>
#include <utility>
namespace AutomationHttpDetail {
nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor) {
  const float LocationX = Actor.Location.X;
  const float LocationY = Actor.Location.Y;
  const float Rotation = Actor.Rotation.Rotation;
  const float Scale = Actor.Scale.Scale;
  if (Actor.ActorId == InvalidActorId || Actor.InstanceName.empty() || Actor.ClassName.empty() ||
      !std::isfinite(LocationX) || !std::isfinite(LocationY) || !std::isfinite(Rotation) ||
      !std::isfinite(Scale)) {
    throw std::runtime_error("Invalid actor snapshot");
  }

  return {
      {"actorId", Actor.ActorId},
      {"instanceName", Actor.InstanceName},
      {"className", Actor.ClassName},
      {"transform",
       {{"location", {{"x", LocationX}, {"y", LocationY}}},
        {"rotation", Rotation},
        {"scale", Scale}}}
  };
}

nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot) {
  nlohmann::json Actors = nlohmann::json::array();
  for (const FAutomationActorSnapshot& Actor : Snapshot.Actors) {
    Actors.push_back(SerializeActor(Actor));
  }
  return {
      {"sceneName", Snapshot.SceneName},
      {"actorCount", Snapshot.Actors.size()},
      {"actors", std::move(Actors)}
  };
}



}
