#include "SpriteActor.h"

#include <stdexcept>

#include "Log.h"
#include "PathResolver.h"
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

const std::string& ASpriteActor::GetImagePath() const { return ImagePath.String(); }

void ASpriteActor::SetImagePath(const std::string& Path) {
  const auto VirtualPath = PathResolver::MakeVirtualPath(Path);
  if (!VirtualPath) {
    M_LOG(Warning, "Invalid image resource path: {}", Path);
    return;
  }
  const FPath OldValue = ImagePath;
  try {
    ImagePath = FPath(*VirtualPath);
  } catch (const std::invalid_argument&) {
    M_LOG(Warning, "Invalid image resource path: {}", Path);
    return;
  }
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
    SetImagePath("/Engine/texture_Checker_64px.png");
  } else {
    OnImagePathChanged(ImagePath);
  }
}
