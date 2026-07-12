#include "SpriteActor.h"

#include "FileUtils.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ASpriteActor);

struct ASpriteActor::Impl {
  std::string ImagePath;
};

ASpriteActor::ASpriteActor() : ImplPtr(new Impl()) {
  SpriteComponent = NewObject<MSpriteComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent) {
    SpriteComponent->RegisterComponent();
  }
}

ASpriteActor::~ASpriteActor() {
  delete ImplPtr;
}

const std::string& ASpriteActor::GetImagePath() const {
  return ImplPtr->ImagePath;
}

void ASpriteActor::SetImagePath(const std::string& path) {
  ImplPtr->ImagePath = FileUtils::GetProjectRelativePath(path);
  if (SpriteComponent) {
    int handle = ResourceManager::GetInstance().LoadResourceGraph(ImplPtr->ImagePath);
    if (handle != -1) {
      SpriteComponent->SubmitGraph(handle, FScale(1.0f), 255);
    }
  }
}

void ASpriteActor::BeginPlay() {
  AActor::BeginPlay();

  if (ImplPtr->ImagePath.empty()) {
    SetImagePath("Engine/EngineSide/Files/texture_Checker_64px.png");
  } else {
    SetImagePath(ImplPtr->ImagePath);
  }
}
