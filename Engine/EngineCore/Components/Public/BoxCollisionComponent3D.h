#pragma once

#include "CollisionComponent3D.h"

class BROCCOLI_ENGINE_API MBoxCollisionComponent3D : public MCollisionComponent3D {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MBoxCollisionComponent3D)

  ECollisionShape3D GetShapeType3D() const override { return ECollisionShape3D::Box; }
  FVector3D GetShapeDimensions3D() const override;
  FVector3D GetHalfExtent() const { return HalfExtent; }
  void SetHalfExtent(const FVector3D& NewHalfExtent);

 private:
  FVector3D HalfExtent{0.5f, 0.5f, 0.5f};
};
