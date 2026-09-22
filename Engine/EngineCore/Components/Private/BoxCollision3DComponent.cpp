#include "BoxCollision3DComponent.h"

#include <algorithm>
#include <cmath>

FVector3D MBoxCollision3DComponent::GetShapeDimensions3D() const {
  const FScale3D Scale = GetWorldScale3D();
  return {
      std::abs(HalfExtent.X * Scale.X),
      std::abs(HalfExtent.Y * Scale.Y),
      std::abs(HalfExtent.Z * Scale.Z)
  };
}

void MBoxCollision3DComponent::SetHalfExtent(const FVector3D& NewHalfExtent) {
  HalfExtent = {
      (std::max)(0.001f, std::abs(NewHalfExtent.X)),
      (std::max)(0.001f, std::abs(NewHalfExtent.Y)),
      (std::max)(0.001f, std::abs(NewHalfExtent.Z))
  };
}
