#pragma once

#include "Actor.h"

class APhysics3DQueryTestActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DQueryTestActor)
  APhysics3DQueryTestActor();
  std::string ObserveQueries();
  std::string ObserveRays();
};

class APhysics3DQueryBoxActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DQueryBoxActor)
  APhysics3DQueryBoxActor();

 protected:
  void BeginPlay() override;
};
