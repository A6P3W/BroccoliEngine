#include "PlayerStart.h"

#include "SceneComponent.h"
#ifdef _EDITOR
#include "SpriteComponent.h"
#endif

#include <memory>

REGISTER_ACTOR(APlayerStart);

APlayerStart::APlayerStart() {
  auto* Root = NewObject<MSceneComponent>(this);
  SetRootComponent(Root);
  if (Root) {
    Root->RegisterComponent();
  }

#ifdef _EDITOR
  auto* Sprite = NewObject<MSpriteComponent>(this);
  if (Sprite) {
    Sprite->SubmitCircle(12.0f, FColor{0, 255, 128, 200}, false);
    Sprite->RegisterComponent();
  }
#endif
}
