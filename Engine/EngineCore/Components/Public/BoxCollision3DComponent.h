#pragma once

#include "Collision3DComponent.h"

class BROCCOLI_ENGINE_API MBoxCollision3DComponent : public MCollision3DComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MBoxCollision3DComponent)

  ECollisionShape3D GetShapeType3D() const override { return ECollisionShape3D::Box; }
  FVector3D GetShapeDimensions3D() const override;
  FVector3D GetHalfExtent() const { return HalfExtent; }
  void SetHalfExtent(const FVector3D& NewHalfExtent);

 private:
  FVector3D HalfExtent{0.5f, 0.5f, 0.5f};
};
