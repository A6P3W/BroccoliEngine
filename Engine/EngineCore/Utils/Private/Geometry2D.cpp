#include "Geometry2D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float Epsilon = 1e-6f;

bool IsWithin(float Value, float Min, float Max) {
  return Value >= Min - Epsilon && Value <= Max + Epsilon;
}
}  // namespace

FRect2D::FRect2D(const FVector2D& InMin, const FVector2D& InMax)
    : Min{(std::min)(InMin.X, InMax.X), (std::min)(InMin.Y, InMax.Y)},
      Max{(std::max)(InMin.X, InMax.X), (std::max)(InMin.Y, InMax.Y)} {}

bool FRect2D::Contains(const FVector2D& Point) const {
  return Point.X >= Min.X && Point.X <= Max.X && Point.Y >= Min.Y && Point.Y <= Max.Y;
}

bool FRect2D::Intersects(const FRect2D& Other) const {
  return !(
      Max.X < Other.Min.X || Min.X > Other.Max.X || Max.Y < Other.Min.Y || Min.Y > Other.Max.Y
  );
}

FVector2D FRect2D::GetSize() const { return Max - Min; }

FVector2D FRect2D::GetCenter() const { return (Min + Max) * 0.5f; }

FRect2D FRect2D::Inset(float Margin) const { return Inset(Margin, Margin); }

FRect2D FRect2D::Inset(float MarginX, float MarginY) const {
  const FVector2D HalfSize = GetSize() * 0.5f;
  const float InsetX = std::clamp(MarginX, 0.0f, HalfSize.X);
  const float InsetY = std::clamp(MarginY, 0.0f, HalfSize.Y);
  return {{Min.X + InsetX, Min.Y + InsetY}, {Max.X - InsetX, Max.Y - InsetY}};
}

bool UGeometry2D::RayIntersectRect(
    const FVector2D& Origin,
    const FVector2D& Direction,
    const FRect2D& Rect,
    FVector2D& OutIntersection
) {
  if (Direction.SizeSquared() < Epsilon * Epsilon) {
    return false;
  }

  float NearestT = std::numeric_limits<float>::infinity();
  const auto ConsiderIntersection = [&](float T, const FVector2D& Point) {
    if (T >= 0.0f && T < NearestT) {
      NearestT = T;
      OutIntersection = Point;
    }
  };

  if (std::abs(Direction.X) >= Epsilon) {
    const float MinT = (Rect.Min.X - Origin.X) / Direction.X;
    const float MinY = Origin.Y + Direction.Y * MinT;
    if (IsWithin(MinY, Rect.Min.Y, Rect.Max.Y)) {
      ConsiderIntersection(MinT, {Rect.Min.X, MinY});
    }

    const float MaxT = (Rect.Max.X - Origin.X) / Direction.X;
    const float MaxY = Origin.Y + Direction.Y * MaxT;
    if (IsWithin(MaxY, Rect.Min.Y, Rect.Max.Y)) {
      ConsiderIntersection(MaxT, {Rect.Max.X, MaxY});
    }
  }

  if (std::abs(Direction.Y) >= Epsilon) {
    const float MinT = (Rect.Min.Y - Origin.Y) / Direction.Y;
    const float MinX = Origin.X + Direction.X * MinT;
    if (IsWithin(MinX, Rect.Min.X, Rect.Max.X)) {
      ConsiderIntersection(MinT, {MinX, Rect.Min.Y});
    }

    const float MaxT = (Rect.Max.Y - Origin.Y) / Direction.Y;
    const float MaxX = Origin.X + Direction.X * MaxT;
    if (IsWithin(MaxX, Rect.Min.X, Rect.Max.X)) {
      ConsiderIntersection(MaxT, {MaxX, Rect.Max.Y});
    }
  }

  return std::isfinite(NearestT);
}
