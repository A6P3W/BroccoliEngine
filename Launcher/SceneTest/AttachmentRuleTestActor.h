#pragma once

#include "Actor.h"

class MSceneComponent;
class MSpriteComponent;

class AAttachmentRuleTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAttachmentRuleTestActor)

  AAttachmentRuleTestActor();

  void OnUpdate(float DeltaTime) override;

 private:
  MSceneComponent* RotatingParent = nullptr;

  MSpriteComponent* KeepRelativeSprite = nullptr;
  MSpriteComponent* KeepWorldSprite = nullptr;
  MSpriteComponent* SnapSprite = nullptr;

  float RotationSpeed = 45.0f;
};
