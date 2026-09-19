#pragma once

#include "Actor.h"

class MBoxCollisionComponent3D;
class MRigidBody3DComponent;
class MSphereCollisionComponent3D;

class APhysics3DStaticFloorActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DStaticFloorActor)
  APhysics3DStaticFloorActor();
};

class APhysics3DDynamicBoxActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicBoxActor)
  APhysics3DDynamicBoxActor();
};

class APhysics3DDynamicSphereActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DDynamicSphereActor)
  APhysics3DDynamicSphereActor();
};

class APhysics3DKinematicActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DKinematicActor)
  APhysics3DKinematicActor();
};
