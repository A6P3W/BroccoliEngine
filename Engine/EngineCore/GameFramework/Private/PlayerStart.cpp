#include "PlayerStart.h"

#include "SceneComponent.h"
#ifdef _EDITOR
#include "SpriteComponent.h"
#endif

#include <memory>

REGISTER_ACTOR(APlayerStart);

APlayerStart::APlayerStart() {
  auto root = std::make_unique<MSceneComponent>();
  auto* rootPtr = root.get();
  SetRootComponent(rootPtr);
  AddComponent(std::move(root));

#ifdef _EDITOR
  auto sprite = std::make_unique<MSpriteComponent>();
  sprite->SubmitCircle(12.0f, 0x00ff80, false, 200);
  AddComponent(std::move(sprite));
#endif
}
