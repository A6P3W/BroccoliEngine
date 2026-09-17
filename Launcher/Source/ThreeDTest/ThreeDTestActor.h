#pragma once

#include "Actor.h"

class MCamera3DComponent;
class MCubeComponent;

class AThreeDTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AThreeDTestActor)

  AThreeDTestActor();
  void OnUpdate(float DeltaTime) override;

 private:
  MCamera3DComponent* Camera = nullptr;
  MCubeComponent* RotatingCube = nullptr;
};
