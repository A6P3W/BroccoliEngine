#pragma once

#include "Actor.h"

class MSpriteComponent;

class ATintTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ATintTestActor)

  ATintTestActor();

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  MSpriteComponent* SpriteComponent = nullptr;
  float ElapsedTime = 0.0f;
};
