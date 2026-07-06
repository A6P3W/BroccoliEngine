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

  auto Sprite = std::make_unique<MSpriteComponent>(9, RenderSpace::World);
  BodySprite = Sprite.get();
  BodySprite->SubmitBox(40.0f, 40.0f, GetColor(255, 150, 40), true);
  BodySprite->SetRelativeLocation({-20.0f, -20.0f});
  AddComponent(std::move(Sprite));

  auto Collision = std::make_unique<MRectangleCollisionComponent>(40.0f, 40.0f);
  Collision->SetParentComponent(GetRootComponent());
  Collision->SetCollisionType(ECollisionType::Overlap);
  AddComponent(std::move(Collision));
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
