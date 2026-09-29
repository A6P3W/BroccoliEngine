#include "SpriteActor.h"

#include "ResourceManager.h"
#include "Sprite2DComponent.h"

REGISTER_ACTOR(ASpriteActor);

ASpriteActor::ASpriteActor() {
  SpriteComponent = NewObject<MSprite2DComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent) {
    SpriteComponent->RegisterComponent();
  }
}

ASpriteActor::~ASpriteActor() = default;

const FPath& ASpriteActor::GetImagePath() const { return ImagePath; }

void ASpriteActor::SetImagePath(const FPath& Path) {
  ImagePath = Path;
  if (SpriteComponent) {
    const int Handle =
        ImagePath.Empty() ? -1 : ResourceManager::GetInstance().LoadResourceGraph(ImagePath);
    SpriteComponent->SubmitGraph(Handle, FScale(1.0f), 255);
  }
}

void ASpriteActor::OnImagePathChanged(FPath OldValue) {
  (void)OldValue;
  SetImagePath(ImagePath);
}

void ASpriteActor::BeginPlay() {
  AActor::BeginPlay();

  if (ImagePath.Empty()) {
    SetImagePath(FPath("/Engine/texture_Checker_64px.png"));
  } else {
    SetImagePath(ImagePath);
  }
}
