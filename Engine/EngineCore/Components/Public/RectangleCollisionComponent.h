#pragma once
#include "BroccoliEngineAPI.h"
#include "CollisionComponent.h"

class BROCCOLI_ENGINE_API MRectangleCollisionComponent : public MCollisionComponent {
 public:
  MRectangleCollisionComponent() = default;

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
