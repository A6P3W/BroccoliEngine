#pragma once

#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MCircleCollision2DComponent;
class MCollision2DComponent;
class MForceFieldComponent;

class BROCCOLI_ENGINE_API AForceFieldActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AForceFieldActor)
  AForceFieldActor();

  MForceFieldComponent* GetForceFieldComponent() const;
  MCollision2DComponent* GetRangeComponent() const;

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  MForceFieldComponent* ForceFieldComponent = nullptr;
  MCircleCollision2DComponent* RangeComponent = nullptr;
};
