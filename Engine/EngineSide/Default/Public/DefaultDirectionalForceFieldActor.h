#pragma once

#include "ForceFieldActor.h"

class ADefaultDirectionalForceFieldActor : public AForceFieldActor {
 public:
  DEFINE_ACTOR_CLASS(ADefaultDirectionalForceFieldActor)
  ADefaultDirectionalForceFieldActor();

 protected:
  void OnUpdate(float DeltaTime) override;
};
