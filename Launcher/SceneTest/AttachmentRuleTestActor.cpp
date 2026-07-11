#include "AttachmentRuleTestActor.h"

#include <DxLib.h>

#include "AttachmentRuleTestComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(AAttachmentRuleTestActor)

AAttachmentRuleTestActor::AAttachmentRuleTestActor() {
  RotatingParent = NewObject<MAttachmentRuleTestComponent>(this);
  RotatingParent->SetRelativeLocation({180.0f, 0.0f});
  RotatingParent->SetRelativeScale(FScale(1.5f));
  RotatingParent->AttachToComponent(GetRootComponent());
  RotatingParent->RegisterComponent();

  auto* ParentMarker = NewObject<MSpriteComponent>(this);
  ParentMarker->SetRenderSettings(10, RenderSpace::World);
  ParentMarker->SubmitBox(48.0f, 48.0f, GetColor(255, 230, 80), true);
  ParentMarker->SetRelativeLocation({-24.0f, -24.0f});
  ParentMarker->AttachToComponent(RotatingParent);
  ParentMarker->RegisterComponent();

  KeepRelativeSprite = NewObject<MSpriteComponent>(this);
  KeepRelativeSprite->SetRenderSettings(11, RenderSpace::World);
  KeepRelativeSprite->SubmitBox(32.0f, 32.0f, GetColor(80, 180, 255), true);
  KeepRelativeSprite->SetRelativeLocation({80.0f, 0.0f});
  KeepRelativeSprite->AttachToComponent(
      RotatingParent, FAttachmentTransformRules::KeepRelativeTransform
  );
  KeepRelativeSprite->RegisterComponent();

  KeepWorldSprite = NewObject<MSpriteComponent>(this);
  KeepWorldSprite->SetRenderSettings(11, RenderSpace::World);
  KeepWorldSprite->SubmitBox(32.0f, 32.0f, GetColor(80, 255, 140), true);
  KeepWorldSprite->SetWorldLocation({-120.0f, 80.0f});
  KeepWorldSprite->AttachToComponent(RotatingParent, FAttachmentTransformRules::KeepWorldTransform);
  KeepWorldSprite->RegisterComponent();

  SnapSprite = NewObject<MSpriteComponent>(this);
  SnapSprite->SetRenderSettings(12, RenderSpace::World);
  SnapSprite->SubmitBox(20.0f, 20.0f, GetColor(255, 90, 120), true);
  SnapSprite->AttachToComponent(
      RotatingParent, FAttachmentTransformRules::SnapToTargetIncludingScale
  );
  SnapSprite->RegisterComponent();
}