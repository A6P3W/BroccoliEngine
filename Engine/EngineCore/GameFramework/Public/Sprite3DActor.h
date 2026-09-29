#pragma once

#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MSprite3DComponent;

class BROCCOLI_ENGINE_API ASprite3DActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASprite3DActor);

  ASprite3DActor();
  ~ASprite3DActor() override;

  void SetImagePath(const FPath& Path);
  const FPath& GetImagePath() const;

 protected:
  void BeginPlay() override;

 private:
  MSprite3DComponent* SpriteComponent = nullptr;
  void OnImagePathChanged(FPath OldValue);
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^ASprite3DActor::OnImagePathChanged, .PathFilter = EPathFilter::Image
  )
  FPath ImagePath;
};
