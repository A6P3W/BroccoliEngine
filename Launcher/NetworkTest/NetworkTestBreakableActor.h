#pragma once

#include "Actor.h"

class MSpriteComponent;

class ANetworkTestBreakableActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ANetworkTestBreakableActor)

  ANetworkTestBreakableActor();

 private:
  void BeginOverlap(AActor* OtherActor) override;

  MSpriteComponent* BodySprite = nullptr;
};
