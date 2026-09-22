#pragma once
#include "BroccoliEngineAPI.h"
#include "Collision2DComponent.h"

class BROCCOLI_ENGINE_API MRectangleCollision2DComponent : public MCollision2DComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MRectangleCollision2DComponent)
  MRectangleCollision2DComponent() = default;

  ECollisionShape GetShapeType() const override { return ECollisionShape::Rectangle; }

  float GetWidth() const { return Width; }
  float GetHeight() const { return Height; }
  void SetSize(float NewWidth, float NewHeight) {
    Width = NewWidth;
    Height = NewHeight;
  }

  void Draw() override;
  FAABB GetAABB() const override;

 private:
  float Width = 100.0f;
  float Height = 100.0f;
};
