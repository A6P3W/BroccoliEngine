#pragma once

#include "Actor.h"

class MCubeComponent;

class ASingleAxisRotationTestActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASingleAxisRotationTestActor)

  ASingleAxisRotationTestActor();
  void OnUpdate(float DeltaTime) override;

 private:
  MCubeComponent* Cube = nullptr;
  float RotationSpeed = 45.0f;
};
