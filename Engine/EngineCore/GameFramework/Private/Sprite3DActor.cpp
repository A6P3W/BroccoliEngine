#include "Sprite3DActor.h"

#include "ResourceManager.h"
#include "Sprite3DComponent.h"

REGISTER_ACTOR(ASprite3DActor);

ASprite3DActor::ASprite3DActor() {
  SpriteComponent = NewObject<MSprite3DComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent) SpriteComponent->RegisterComponent();
}

ASprite3DActor::~ASprite3DActor() = default;

void ASprite3DActor::SetImagePath(const FPath& Path) {
  ImagePath = Path;
  if (SpriteComponent == nullptr) return;
  const int Handle =
      ImagePath.Empty() ? 0 : ResourceManager::GetInstance().LoadResourceGraph(ImagePath);
  SpriteComponent->SetTexture(Handle);
}

const FPath& ASprite3DActor::GetImagePath() const { return ImagePath; }

void ASprite3DActor::OnImagePathChanged(FPath OldValue) {
  (void)OldValue;
  SetImagePath(ImagePath);
}

void ASprite3DActor::BeginPlay() {
  AActor::BeginPlay();
  if (ImagePath.Empty()) {
    SetImagePath(FPath("/Engine/texture_Checker_64px.png"));
  } else {
    SetImagePath(ImagePath);
  }
}
