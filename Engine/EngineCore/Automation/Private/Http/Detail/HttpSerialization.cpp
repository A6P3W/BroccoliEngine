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
  const FVector3D& Location3D = Actor.Location3D;
  const FQuaternion& Rotation3D = Actor.Rotation3D;
  const FScale3D& Scale3D = Actor.Scale3D;
  if (Actor.ActorId == InvalidActorId || Actor.InstanceName.empty() || Actor.ClassName.empty() ||
      !std::isfinite(LocationX) || !std::isfinite(LocationY) || !std::isfinite(Rotation) ||
      !std::isfinite(Scale) || !std::isfinite(Location3D.X) || !std::isfinite(Location3D.Y) ||
      !std::isfinite(Location3D.Z) || !std::isfinite(Rotation3D.X) ||
      !std::isfinite(Rotation3D.Y) || !std::isfinite(Rotation3D.Z) ||
      !std::isfinite(Rotation3D.W) || !std::isfinite(Scale3D.X) || !std::isfinite(Scale3D.Y) ||
      !std::isfinite(Scale3D.Z)) {
    throw std::runtime_error("Invalid actor snapshot");
  }

  return {
      {"actorId", Actor.ActorId},
      {"instanceName", Actor.InstanceName},
      {"className", Actor.ClassName},
      {"transform",
       {{"location", {{"x", LocationX}, {"y", LocationY}}},
        {"rotation", Rotation},
        {"scale", Scale},
        {"location3D", {{"x", Location3D.X}, {"y", Location3D.Y}, {"z", Location3D.Z}}},
        {"rotation3D",
         {{"x", Rotation3D.X}, {"y", Rotation3D.Y}, {"z", Rotation3D.Z}, {"w", Rotation3D.W}}},
        {"scale3D", {{"x", Scale3D.X}, {"y", Scale3D.Y}, {"z", Scale3D.Z}}}}}
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
