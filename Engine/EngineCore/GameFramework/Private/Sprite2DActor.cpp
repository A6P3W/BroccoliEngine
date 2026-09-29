#include "Sprite2DActor.h"

#include "ResourceManager.h"
#include "Sprite2DComponent.h"

REGISTER_ACTOR(ASprite2DActor);

ASprite2DActor::ASprite2DActor() {
  SpriteComponent = NewObject<MSprite2DComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent) {
    SpriteComponent->RegisterComponent();
  }
}

ASprite2DActor::~ASprite2DActor() = default;

const FPath& ASprite2DActor::GetImagePath() const { return ImagePath; }

void ASprite2DActor::SetImagePath(const FPath& Path) {
  ImagePath = Path;
  if (SpriteComponent) {
    const int Handle =
        ImagePath.Empty() ? 0 : ResourceManager::GetInstance().LoadResourceGraph(ImagePath);
    SpriteComponent->SubmitGraph(Handle, FScale(1.0f), 255);
  }
}

void ASprite2DActor::OnImagePathChanged(FPath OldValue) {
  (void)OldValue;
  SetImagePath(ImagePath);
}

void ASprite2DActor::BeginPlay() {
  AActor::BeginPlay();

  if (ImagePath.Empty()) {
    SetImagePath(FPath("/Engine/texture_Checker_64px.png"));
  } else {
    SetImagePath(ImagePath);
  }
}
