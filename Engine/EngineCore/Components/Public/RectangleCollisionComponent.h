#pragma once
#include "CollisionComponent.h"

class MRectangleCollisionComponent : public MCollisionComponent {
 public:
  MRectangleCollisionComponent(float width = 100.0f, float height = 100.0f)
      : Width(width), Height(height) {}

  ECollisionShape GetShapeType() const override { return ECollisionShape::Rectangle; }

  float GetWidth() const { return Width; }
  float GetHeight() const { return Height; }

  void Draw() override;
  FAABB GetAABB() const override;

 private:
  float Width;
  float Height;
};
