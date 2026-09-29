#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"

class MSprite2DComponent;

class BROCCOLI_ENGINE_API ASprite2DActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASprite2DActor);

  ASprite2DActor();
  ~ASprite2DActor() override;

  void SetImagePath(const FPath& Path);
  const FPath& GetImagePath() const;

 protected:
  void BeginPlay() override;

 private:
  MSprite2DComponent* SpriteComponent = nullptr;
  void OnImagePathChanged(FPath OldValue);
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^ASprite2DActor::OnImagePathChanged, .PathFilter = EPathFilter::Image
  )
  FPath ImagePath;
};
