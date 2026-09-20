#include "SphereCollisionComponent3D.h"

#include <algorithm>
#include <cmath>

FVector3D MSphereCollisionComponent3D::GetShapeDimensions3D() const {
  const FScale3D Scale = GetWorldScale3D();
  const float LargestScale = (std::max)({std::abs(Scale.X), std::abs(Scale.Y), std::abs(Scale.Z)});
  return {Radius * LargestScale, 0.0f, 0.0f};
}

void MSphereCollisionComponent3D::SetRadius(float NewRadius) {
  Radius = (std::max)(0.001f, std::abs(NewRadius));
}
