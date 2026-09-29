#pragma once

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "SceneComponent.h"
#include "SpriteBillboardMode.h"

class BROCCOLI_ENGINE_API MSprite3DComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSprite3DComponent)

  void SetTexture(int NewTextureHandle);
  int GetTexture() const;
  void SetSpriteSize(const FVector2D& Size);
  const FVector2D& GetSpriteSize() const;
  void SetPivot(const FVector2D& NewPivot);
  const FVector2D& GetPivot() const;
  void SetSpriteRotation(float Degrees);
  float GetSpriteRotation() const;
  void SetBillboardMode(ESpriteBillboardMode Mode);
  ESpriteBillboardMode GetBillboardMode() const;
  void SetTint(const FColor& NewTint);
  const FColor& GetTint() const;
  void Draw() override;

 private:
  int TextureHandle = 0;
  FVector2D SpriteSize{1.0f, 1.0f};
  FVector2D Pivot{0.5f, 0.5f};
  float SpriteRotationDegrees = 0.0f;
  ESpriteBillboardMode BillboardMode = ESpriteBillboardMode::None;
  FColor Tint = FColor::White;
};
