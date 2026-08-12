#pragma once

#include "BroccoliEngineAPI.h"
#include "UMath.h"

struct BROCCOLI_ENGINE_API FRect2D {
  FVector2D Min = FVector2D::ZeroVector();
  FVector2D Max = FVector2D::ZeroVector();

  FRect2D() = default;
  FRect2D(const FVector2D& InMin, const FVector2D& InMax);

  bool Contains(const FVector2D& Point) const;
  bool Intersects(const FRect2D& Other) const;

  FVector2D GetSize() const;
  FVector2D GetCenter() const;

  FRect2D Inset(float Margin) const;
  FRect2D Inset(float MarginX, float MarginY) const;
};

class BROCCOLI_ENGINE_API UGeometry2D {
 public:
  static bool RayIntersectRect(
      const FVector2D& Origin,
      const FVector2D& Direction,
      const FRect2D& Rect,
      FVector2D& OutIntersection
  );
};
