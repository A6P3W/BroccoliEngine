#include "NetworkTestBreakableActor.h"

#include <DxLib.h>
#include <memory>

#include "CollisionComponent.h"
#include "Log.h"
#include "NetworkTestPawn.h"
#include "RectangleCollisionComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ANetworkTestBreakableActor)

ANetworkTestBreakableActor::ANetworkTestBreakableActor() {
  bReplicates = true;

  BodySprite = NewObject<MSpriteComponent>(this);
  BodySprite->SetRenderSettings(9, RenderSpace::World);
  BodySprite->SubmitBox(40.0f, 40.0f, GetColor(255, 150, 40), true);
  BodySprite->SetRelativeLocation({-20.0f, -20.0f});
  BodySprite->RegisterComponent();

  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  Collision->SetSize(40.0f, 40.0f);
  Collision->SetParentComponent(GetRootComponent());
  Collision->SetCollisionType(ECollisionType::Overlap);
  Collision->RegisterComponent();
}

void ANetworkTestBreakableActor::BeginOverlap(AActor* OtherActor) {
  AActor::BeginOverlap(OtherActor);

  if (!bHasAuthority || IsPendingDestroy()) {
    return;
  }

  if (dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    M_LOG("NetworkTestBreakableActor destroyed by player overlap: actor={}", NetworkId);
    Destroy();
  }
}
