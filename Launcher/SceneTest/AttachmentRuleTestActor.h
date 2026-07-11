#pragma once

#include "Actor.h"

class MAttachmentRuleTestComponent;
class MSpriteComponent;

class AAttachmentRuleTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAttachmentRuleTestActor)

  AAttachmentRuleTestActor();

 private:
  MAttachmentRuleTestComponent* RotatingParent = nullptr;
  MSpriteComponent* KeepRelativeSprite = nullptr;
  MSpriteComponent* KeepWorldSprite = nullptr;
  MSpriteComponent* SnapSprite = nullptr;
};