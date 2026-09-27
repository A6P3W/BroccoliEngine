#include "SpriteActor.h"

#include "ResourceManager.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ASpriteActor);

ASpriteActor::ASpriteActor() {
  SpriteComponent = NewObject<MSpriteComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent) {
    SpriteComponent->RegisterComponent();
  }
}

ASpriteActor::~ASpriteActor() = default;

const FPath& ASpriteActor::GetImagePath() const { return ImagePath; }

void ASpriteActor::SetImagePath(const FPath& Path) {
  const FPath OldValue = ImagePath;
  ImagePath = Path;
  if (ImagePath == OldValue) return;
  OnImagePathChanged(OldValue);
}

void ASpriteActor::OnImagePathChanged(FPath OldValue) {
  (void)OldValue;
  if (SpriteComponent) {
    const int Handle = ImagePath.Empty()
                           ? -1
                           : ResourceManager::GetInstance().LoadResourceGraph(ImagePath.String());
    SpriteComponent->SubmitGraph(Handle, FScale(1.0f), 255);
  }
}

void ASpriteActor::BeginPlay() {
  AActor::BeginPlay();

  if (ImagePath.Empty()) {
    SetImagePath(FPath("/Engine/texture_Checker_64px.png"));
  } else {
    OnImagePathChanged(ImagePath);
  }
}
