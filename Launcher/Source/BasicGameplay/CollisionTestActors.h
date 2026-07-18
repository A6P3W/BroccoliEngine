#pragma once

#include "Actor.h"

class MCollisionComponent;

// Base class shared by the three shape actors.  Their collision components use
// the engine's debug Draw implementation, making shape/rotation/scale visible.
class ACollisionTestActorBase : public AActor {
 public:
  void OnUpdate(float DeltaTime) override;
  void BeginOverlap(AActor* OtherActor) override;
  void EndOverlap(AActor* OtherActor) override;

 protected:
  void ConfigureCollision(MCollisionComponent* Collision, const char* ShapeName);

 private:
  const char* ShapeLabel = "Collision";
  float ElapsedSeconds = 0.0f;
};

class ACollisionTestRectangleActor : public ACollisionTestActorBase {
 public:
  DEFINE_ACTOR_CLASS(ACollisionTestRectangleActor)
  ACollisionTestRectangleActor();
};

class ACollisionTestBlockRectangleActor : public ACollisionTestActorBase {
 public:
  DEFINE_ACTOR_CLASS(ACollisionTestBlockRectangleActor)
  ACollisionTestBlockRectangleActor();
};
class ACollisionTestCircleActor : public ACollisionTestActorBase {
 public:
  DEFINE_ACTOR_CLASS(ACollisionTestCircleActor)
  ACollisionTestCircleActor();
};

class ACollisionTestLineActor : public ACollisionTestActorBase {
 public:
  DEFINE_ACTOR_CLASS(ACollisionTestLineActor)
  ACollisionTestLineActor();
};
