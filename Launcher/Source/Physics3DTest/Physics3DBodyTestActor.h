#pragma once

#include <nlohmann/json.hpp>

#include "Actor.h"
#include "AutomationAnnotations.h"

class MBoxCollision3DComponent;
class MRigidBody3DComponent;
class MSphereCollision3DComponent;

struct FPhysics3DCollisionObservation {
  uint32_t BeginOverlapCount = 0;
  uint32_t EndOverlapCount = 0;
  std::vector<FActorId> OverlappingActorIds;
};

class APhysics3DStaticFloorActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DStaticFloorActor)
  APhysics3DStaticFloorActor();

 protected:
  void BeginPlay() override;
};

class APhysics3DDynamicBoxActor final : public AActor {
 private:
  static nlohmann::json ObservationToJson(const FPhysics3DCollisionObservation& Observation);

 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicBoxActor)
  APhysics3DDynamicBoxActor();
  CONTROL_METHOD(
          .Name = "get_collision_observation",
          .Description = "Returns 3D overlap event counts and current overlap actor IDs.",
          .ResultAdapter = ^^APhysics3DDynamicBoxActor::ObservationToJson
  )
  FPhysics3DCollisionObservation GetCollisionObservation() const;

 protected:
  void BeginPlay() override;
  void BeginOverlap(AActor* OtherActor) override;
  void EndOverlap(AActor* OtherActor) override;

 private:
  uint32_t BeginOverlapCount = 0;
  uint32_t EndOverlapCount = 0;
  std::vector<FActorId> OverlappingActorIds;
};

class APhysics3DDynamicSphereActor final : public AActor {
 private:
  static nlohmann::json ObservationToJson(const FPhysics3DCollisionObservation& Observation);

 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicSphereActor)
  APhysics3DDynamicSphereActor();
  CONTROL_METHOD(
          .Name = "get_collision_observation",
          .Description = "Returns 3D overlap event counts and current overlap actor IDs.",
          .ResultAdapter = ^^APhysics3DDynamicSphereActor::ObservationToJson
  )
  FPhysics3DCollisionObservation GetCollisionObservation() const;

 protected:
  void BeginPlay() override;
  void BeginOverlap(AActor* OtherActor) override;
  void EndOverlap(AActor* OtherActor) override;

 private:
  uint32_t BeginOverlapCount = 0;
  uint32_t EndOverlapCount = 0;
  std::vector<FActorId> OverlappingActorIds;
};

class APhysics3DKinematicActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DKinematicActor)
  APhysics3DKinematicActor();

 protected:
  void BeginPlay() override;
};
