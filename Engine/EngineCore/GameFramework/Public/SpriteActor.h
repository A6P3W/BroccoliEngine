#pragma once
#include "BroccoliEngineAPI.h"
#include <string>

#include "Actor.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API ASpriteActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASpriteActor);

  ASpriteActor();
  ~ASpriteActor() override;

  void SetImagePath(const std::string& path);
  const std::string& GetImagePath() const;

 protected:
  void BeginPlay() override;

 private:
  MSpriteComponent* SpriteComponent = nullptr;
  struct Impl;
  Impl* ImplPtr = nullptr;
};
