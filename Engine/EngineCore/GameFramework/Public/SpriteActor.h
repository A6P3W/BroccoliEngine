#pragma once
#include <string>

#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API ASpriteActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASpriteActor);

  ASpriteActor();
  ~ASpriteActor() override;

  void SetImagePath(const std::string& Path);
  const std::string& GetImagePath() const;

 protected:
  void BeginPlay() override;

 private:
  MSpriteComponent* SpriteComponent = nullptr;
  void OnImagePathChanged(FPath OldValue);
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^ASpriteActor::OnImagePathChanged, .PathFilter = EPathFilter::Image
  )
  FPath ImagePath;
};
