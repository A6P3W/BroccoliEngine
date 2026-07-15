#pragma once

#include "ForceFieldActor.h"

class MSpriteComponent;

class ANetworkTestForceFieldActor : public AForceFieldActor {
 public:
  DEFINE_ACTOR_CLASS(ANetworkTestForceFieldActor)
  ANetworkTestForceFieldActor();

 private:
  MSpriteComponent* FieldMarker = nullptr;
};
