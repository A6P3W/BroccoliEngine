#include "Sprite3DComponent.h"

#include <algorithm>

#include "RenderSystem.h"

void MSprite3DComponent::SetTexture(int NewTextureHandle) { TextureHandle = NewTextureHandle; }
int MSprite3DComponent::GetTexture() const { return TextureHandle; }
void MSprite3DComponent::SetSpriteSize(const FVector2D& Size) { SpriteSize = Size; }
const FVector2D& MSprite3DComponent::GetSpriteSize() const { return SpriteSize; }
void MSprite3DComponent::SetPivot(const FVector2D& NewPivot) {
  Pivot = {std::clamp(NewPivot.X, 0.0f, 1.0f), std::clamp(NewPivot.Y, 0.0f, 1.0f)};
}
const FVector2D& MSprite3DComponent::GetPivot() const { return Pivot; }
void MSprite3DComponent::SetSpriteRotation(float Degrees) { SpriteRotationDegrees = Degrees; }
float MSprite3DComponent::GetSpriteRotation() const { return SpriteRotationDegrees; }
void MSprite3DComponent::SetBillboardMode(ESpriteBillboardMode Mode) { BillboardMode = Mode; }
ESpriteBillboardMode MSprite3DComponent::GetBillboardMode() const { return BillboardMode; }
void MSprite3DComponent::SetTint(const FColor& NewTint) { Tint = NewTint; }
const FColor& MSprite3DComponent::GetTint() const { return Tint; }
void MSprite3DComponent::Draw() {
  if (!IsVisible() || TextureHandle <= 0 || SpriteSize.X <= 0.0f || SpriteSize.Y <= 0.0f) return;
  RenderSystem::GetInstance().SubmitSprite3D(
      GetWorldTransform3D(),
      TextureHandle,
      SpriteSize,
      Pivot,
      SpriteRotationDegrees,
      BillboardMode,
      Tint
  );
}
