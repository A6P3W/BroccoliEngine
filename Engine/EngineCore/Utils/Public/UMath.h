#pragma once

#include "BroccoliEngineAPI.h"

#include <cmath>
#include <compare>
#include <numbers>

class UMath {
 public:
  static constexpr float DegToRad(float Deg) { return Deg * (std::numbers::pi_v<float> / 180.0f); }

  static constexpr float RadToDeg(float Rad) { return Rad * (180.0f / std::numbers::pi_v<float>); }
};

struct FRotator;

struct BROCCOLI_ENGINE_API FScale {
  float Scale = 1.0f;

  FScale() = default;

  explicit FScale(float InScale) : Scale(InScale) {}

  // --------------------
  // Scale OP Scale
  // --------------------

  FScale operator+(const FScale& Other) const { return FScale(Scale + Other.Scale); }

  FScale operator-(const FScale& Other) const { return FScale(Scale - Other.Scale); }

  FScale operator*(const FScale& Other) const { return FScale(Scale * Other.Scale); }

  FScale operator/(const FScale& Other) const { return FScale(Scale / Other.Scale); }

  FScale& operator+=(const FScale& Other) {
    Scale += Other.Scale;
    return *this;
  }

  FScale& operator-=(const FScale& Other) {
    Scale -= Other.Scale;
    return *this;
  }

  FScale& operator*=(const FScale& Other) {
    Scale *= Other.Scale;
    return *this;
  }

  FScale& operator/=(const FScale& Other) {
    Scale /= Other.Scale;
    return *this;
  }

  // --------------------
  // Scale OP Float
  // --------------------

  FScale operator+(float Value) const { return FScale(Scale + Value); }

  FScale operator-(float Value) const { return FScale(Scale - Value); }

  FScale operator*(float Value) const { return FScale(Scale * Value); }

  FScale operator/(float Value) const { return FScale(Scale / Value); }

  FScale& operator+=(float Value) {
    Scale += Value;
    return *this;
  }

  FScale& operator-=(float Value) {
    Scale -= Value;
    return *this;
  }

  FScale& operator*=(float Value) {
    Scale *= Value;
    return *this;
  }

  FScale& operator/=(float Value) {
    Scale /= Value;
    return *this;
  }

  auto operator<=>(const FScale&) const = default;
};

struct BROCCOLI_ENGINE_API FVector2D {
  float X = 0.0f;
  float Y = 0.0f;

  inline constexpr FVector2D() = default;

  inline constexpr FVector2D(float InX, float InY) : X(InX), Y(InY) {}

  inline static constexpr FVector2D ZeroVector() { return FVector2D(0.0f, 0.0f); }

  float SizeSquared() const { return X * X + Y * Y; }

  float Size() const { return std::sqrt(SizeSquared()); }

  bool Equals(const FVector2D& Other, float Tolerance = 0.001f) const {
    return std::abs(X - Other.X) < Tolerance && std::abs(Y - Other.Y) < Tolerance;
  }

  // --------------------
  // Vector OP Vector
  // --------------------

  FVector2D operator+(const FVector2D& Other) const { return {X + Other.X, Y + Other.Y}; }

  FVector2D operator-(const FVector2D& Other) const { return {X - Other.X, Y - Other.Y}; }

  FVector2D& operator+=(const FVector2D& Other) {
    X += Other.X;
    Y += Other.Y;
    return *this;
  }

  FVector2D& operator-=(const FVector2D& Other) {
    X -= Other.X;
    Y -= Other.Y;
    return *this;
  }

  // --------------------
  // Vector OP Scalar
  // --------------------

  FVector2D operator+(float Value) const { return {X + Value, Y + Value}; }

  FVector2D operator-(float Value) const { return {X - Value, Y - Value}; }

  inline FVector2D operator*(float Value) const { return {X * Value, Y * Value}; }

  FVector2D operator/(float Value) const { return {X / Value, Y / Value}; }

  FVector2D& operator+=(float Value) {
    X += Value;
    Y += Value;
    return *this;
  }

  FVector2D& operator-=(float Value) {
    X -= Value;
    Y -= Value;
    return *this;
  }

  FVector2D& operator*=(float Value) {
    X *= Value;
    Y *= Value;
    return *this;
  }

  FVector2D& operator/=(float Value) {
    X /= Value;
    Y /= Value;
    return *this;
  }
  FVector2D operator*(const FScale& InScale) const {
    return {X * InScale.Scale, Y * InScale.Scale};
  }

  FVector2D& operator*=(const FScale& InScale) {
    X *= InScale.Scale;
    Y *= InScale.Scale;
    return *this;
  }
  FVector2D operator/(const FScale& InScale) const {
    return {X / InScale.Scale, Y / InScale.Scale};
  }

  FVector2D& operator/=(const FScale& InScale) {
    X /= InScale.Scale;
    Y /= InScale.Scale;
    return *this;
  }
  auto operator<=>(const FVector2D&) const = default;

  FVector2D RotateVector(const FRotator& Angle) const;
};

struct BROCCOLI_ENGINE_API FRotator {
  float Rotation = 0.0f;

  FRotator() = default;

  explicit FRotator(float InRotation) : Rotation(InRotation) {}

  FRotator operator+(const FRotator& Other) const { return FRotator(Rotation + Other.Rotation); }

  FRotator operator-(const FRotator& Other) const { return FRotator(Rotation - Other.Rotation); }

  FRotator& operator+=(const FRotator& Other) {
    Rotation += Other.Rotation;
    return *this;
  }

  FRotator& operator-=(const FRotator& Other) {
    Rotation -= Other.Rotation;
    return *this;
  }

  auto operator<=>(const FRotator&) const = default;
};

inline FVector2D FVector2D::RotateVector(const FRotator& Angle) const {
  const float Rad = UMath::DegToRad(Angle.Rotation);

  const float CosTheta = std::cos(Rad);
  const float SinTheta = std::sin(Rad);

  return {X * CosTheta - Y * SinTheta, X * SinTheta + Y * CosTheta};
}


