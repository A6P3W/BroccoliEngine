#pragma once

#include "Actor.h"
#include "ReflectionGenerator.h"

class AMovingPlatformActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AMovingPlatformActor)

 protected:
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
  EDITOR_PROPERTY(.Min = 0.0f, .SliderMin = 0.0f, .SliderMax = 500.0f)
  float Speed = 100.0f;

  EDITOR_PROPERTY(.Min = 0.0f)
  float Distance = 300.0f;

  EDITOR_PROPERTY()
  FVector3D Direction{1.0f, 0.0f, 0.0f};

  EDITOR_PROPERTY()
  bool Enabled = true;

  FVector3D StartLocation{};
  float Travel = 0.0f;
};
