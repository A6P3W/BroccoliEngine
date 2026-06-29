#pragma once
#include <string>

#include "Actor.h"

class MSpriteComponent;

class ASpriteActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASpriteActor);

  ASpriteActor();

  void SetImagePath(const std::string& path);
  const std::string& GetImagePath() const { return ImagePath; }

 protected:
  void BeginPlay() override;

 private:
  MSpriteComponent* SpriteComponent = nullptr;
  std::string ImagePath;
};
