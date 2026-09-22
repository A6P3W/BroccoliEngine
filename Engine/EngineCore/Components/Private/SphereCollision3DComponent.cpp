#include "SphereCollision3DComponent.h"

#include <algorithm>
#include <cmath>

FVector3D MSphereCollision3DComponent::GetShapeDimensions3D() const {
  const FScale3D Scale = GetWorldScale3D();
  const float LargestScale = (std::max)({std::abs(Scale.X), std::abs(Scale.Y), std::abs(Scale.Z)});
  return {Radius * LargestScale, 0.0f, 0.0f};
}

void MSphereCollision3DComponent::SetRadius(float NewRadius) {
  Radius = (std::max)(0.001f, std::abs(NewRadius));
}
