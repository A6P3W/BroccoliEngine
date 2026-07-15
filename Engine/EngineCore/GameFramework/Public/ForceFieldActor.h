#pragma once

#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MCircleCollisionComponent;
class MCollisionComponent;
class MForceFieldComponent;

class BROCCOLI_ENGINE_API AForceFieldActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AForceFieldActor)
  AForceFieldActor();

  MForceFieldComponent* GetForceFieldComponent() const;
  MCollisionComponent* GetRangeComponent() const;

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  MForceFieldComponent* ForceFieldComponent = nullptr;
  MCircleCollisionComponent* RangeComponent = nullptr;
};
