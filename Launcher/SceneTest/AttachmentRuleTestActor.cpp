#include "AttachmentRuleTestActor.h"

#include <DxLib.h>

#include "SceneComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(AAttachmentRuleTestActor)

AAttachmentRuleTestActor::AAttachmentRuleTestActor() {
  // ============================================================
  // 回転する親
  // ============================================================

  RotatingParent = NewObject<MSceneComponent>(this);
  RotatingParent->SetRelativeLocation({320.0f, 240.0f});
  RotatingParent->SetRelativeScale(FScale(1.5f));
  RotatingParent->AttachToComponent(GetRootComponent());
  RotatingParent->RegisterComponent();

  // 親の中心を示す黄色い円
  auto* ParentMarker = NewObject<MSpriteComponent>(this);
  ParentMarker->SetRenderSettings(10, RenderSpace::World);
  ParentMarker->SubmitCircle(18.0f, FColor{255, 220, 60}, true);
  ParentMarker->AttachToComponent(RotatingParent);
  ParentMarker->RegisterComponent();

  // ============================================================
  // KeepRelativeTransform
  //
  // 相対位置 (100, 0) をそのまま維持する。
  //
  // 親スケールが1.5なので、実際の距離は150になる。
  // 親が回転すると、青い円は親の周囲を回転する。
  // ============================================================

  KeepRelativeSprite = NewObject<MSpriteComponent>(this);
  KeepRelativeSprite->SetRenderSettings(11, RenderSpace::World);
  KeepRelativeSprite->SubmitCircle(12.0f, FColor{80, 170, 255}, true);

  KeepRelativeSprite->SetRelativeLocation({100.0f, 0.0f});

  KeepRelativeSprite->AttachToComponent(
      RotatingParent, FAttachmentTransformRules::KeepRelativeTransform
  );

  KeepRelativeSprite->RegisterComponent();

  // ============================================================
  // KeepWorldTransform
  //
  // アタッチ前のワールド位置とワールドスケールを維持する。
  //
  // 親にアタッチした瞬間は、緑の円の見た目が変化しない。
  // アタッチ後は親の子なので、親が回転すると一緒に回転する。
  // ============================================================

  const FVector2D KeepWorldStartLocation{170.0f, 240.0f};

  // アタッチ前の位置を示す灰色の円
  auto* KeepWorldStartMarker = NewObject<MSpriteComponent>(this);

  KeepWorldStartMarker->SetRenderSettings(5, RenderSpace::World);

  KeepWorldStartMarker->SubmitCircle(16.0f, FColor{160, 160, 160}, false);

  KeepWorldStartMarker->SetWorldLocation(KeepWorldStartLocation);

  KeepWorldStartMarker->RegisterComponent();

  KeepWorldSprite = NewObject<MSpriteComponent>(this);
  KeepWorldSprite->SetRenderSettings(11, RenderSpace::World);
  KeepWorldSprite->SubmitCircle(12.0f, FColor{80, 255, 140}, true);

  // アタッチ前にワールド値を指定
  KeepWorldSprite->SetWorldLocation(KeepWorldStartLocation);

  KeepWorldSprite->SetWorldScale(FScale(0.75f));

  KeepWorldSprite->AttachToComponent(RotatingParent, FAttachmentTransformRules::KeepWorldTransform);

  KeepWorldSprite->RegisterComponent();

  // ============================================================
  // SnapToTargetIncludingScale
  //
  // アタッチ前の位置、回転、スケールを無視する。
  // 親の中心へ移動し、親と同じ回転・スケールになる。
  // ============================================================

  const FVector2D SnapBeforeLocation{500.0f, 100.0f};

  // Snap前の位置を示す灰色の円
  auto* SnapBeforeMarker = NewObject<MSpriteComponent>(this);

  SnapBeforeMarker->SetRenderSettings(5, RenderSpace::World);

  SnapBeforeMarker->SubmitCircle(14.0f, FColor{160, 160, 160}, false);

  SnapBeforeMarker->SetWorldLocation(SnapBeforeLocation);

  SnapBeforeMarker->RegisterComponent();

  SnapSprite = NewObject<MSpriteComponent>(this);
  SnapSprite->SetRenderSettings(12, RenderSpace::World);
  SnapSprite->SubmitCircle(10.0f, FColor{255, 80, 110}, true);

  // この位置はSnapによって無視される
  SnapSprite->SetWorldLocation(SnapBeforeLocation);

  SnapSprite->SetWorldScale(FScale(0.5f));

  SnapSprite->AttachToComponent(
      RotatingParent, FAttachmentTransformRules::SnapToTargetIncludingScale
  );

  SnapSprite->RegisterComponent();
}

void AAttachmentRuleTestActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);

  if (RotatingParent == nullptr) {
    return;
  }

  // 親を毎秒45度回転させる
  RotatingParent->AddWorldRotation(FRotator(RotationSpeed * DeltaTime));
}
