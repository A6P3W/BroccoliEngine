#include "HttpSerialization.h"

#include <cmath>
#include <stdexcept>
#include <utility>
namespace AutomationHttpDetail {
nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor) {
  const FVector3D& Location = Actor.Location;
  const FRotator3D& Rotation = Actor.Rotation;
  const FScale3D& Scale = Actor.Scale;
  if (Actor.ActorId == InvalidActorId || Actor.InstanceName.empty() || Actor.ClassName.empty() ||
      !std::isfinite(Location.X) || !std::isfinite(Location.Y) || !std::isfinite(Location.Z) ||
      !std::isfinite(Rotation.Pitch) || !std::isfinite(Rotation.Yaw) ||
      !std::isfinite(Rotation.Roll) || !std::isfinite(Scale.X) || !std::isfinite(Scale.Y) ||
      !std::isfinite(Scale.Z)) {
    throw std::runtime_error("Invalid actor snapshot");
  }

  return {
      {"actorId", Actor.ActorId},
      {"instanceName", Actor.InstanceName},
      {"className", Actor.ClassName},
      {"transform",
       {{"location", {{"x", Location.X}, {"y", Location.Y}, {"z", Location.Z}}},
        {"rotation", {{"pitch", Rotation.Pitch}, {"yaw", Rotation.Yaw}, {"roll", Rotation.Roll}}},
        {"scale", {{"x", Scale.X}, {"y", Scale.Y}, {"z", Scale.Z}}}}}
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

}  // namespace AutomationHttpDetail
