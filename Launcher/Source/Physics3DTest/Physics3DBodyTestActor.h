#pragma once

#include "Actor.h"

class MBoxCollisionComponent3D;
class MRigidBody3DComponent;
class MSphereCollisionComponent3D;

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
 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicBoxActor)
  APhysics3DDynamicBoxActor();
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
 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicSphereActor)
  APhysics3DDynamicSphereActor();
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
