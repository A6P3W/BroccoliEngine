#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MSprite2DComponent;

class BROCCOLI_ENGINE_API ASpriteActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASpriteActor);

  ASpriteActor();
  ~ASpriteActor() override;

  void SetImagePath(const FPath& Path);
  const FPath& GetImagePath() const;

 protected:
  void BeginPlay() override;

 private:
  MSprite2DComponent* SpriteComponent = nullptr;
  void OnImagePathChanged(FPath OldValue);
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^ASpriteActor::OnImagePathChanged, .PathFilter = EPathFilter::Image
  )
  FPath ImagePath;
};
