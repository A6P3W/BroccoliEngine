#include "SpriteActor.h"

#include "FileUtils.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ASpriteActor);

ASpriteActor::ASpriteActor() {
  auto spriteComp = std::make_unique<MSpriteComponent>();
  SpriteComponent = spriteComp.get();

  SetRootComponent(SpriteComponent);
  AddComponent(std::move(spriteComp));
}

void ASpriteActor::SetImagePath(const std::string& path) {
  ImagePath = FileUtils::GetProjectRelativePath(path);
  if (SpriteComponent) {
    int handle = ResourceManager::GetInstance().LoadResourceGraph(ImagePath);
    if (handle != -1) {
      SpriteComponent->SubmitGraph(handle, FScale(1.0f), 255);
    }
  }
}

void ASpriteActor::BeginPlay() {
  AActor::BeginPlay();

  if (ImagePath.empty()) {
    SetImagePath("Engine/EngineSide/Files/texture_Checker_64px.png");
  } else {
    SetImagePath(ImagePath);
  }
}
