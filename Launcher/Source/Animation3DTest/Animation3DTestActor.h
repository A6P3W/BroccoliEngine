#pragma once

#include "Actor.h"

class MSkeletalMeshComponent;

class AAnimation3DTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAnimation3DTestActor)
  AAnimation3DTestActor();

 protected:
  void BeginPlay() override;

 private:
  MSkeletalMeshComponent* Mesh = nullptr;
  EDITOR_PROPERTY()
  std::string AnimationName = "Walk";
  EDITOR_PROPERTY()
  float Speed = 1.0f;
};

class AAnimation3DTestCameraActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAnimation3DTestCameraActor)
  AAnimation3DTestCameraActor();
};
