#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <numbers>

#include "BroccoliEngineAPI.h"

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

struct BROCCOLI_ENGINE_API FVector3D {
  float X = 0.0f;
  float Y = 0.0f;
  float Z = 0.0f;

  constexpr FVector3D() = default;
  constexpr FVector3D(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
  static constexpr FVector3D ZeroVector() { return {}; }
  float SizeSquared() const { return X * X + Y * Y + Z * Z; }
  float Size() const { return std::sqrt(SizeSquared()); }
  FVector3D Normalize() const {
    const float Length = Size();
    return Length > 1e-6f ? *this / Length : ZeroVector();
  }
  float Dot(const FVector3D& Other) const { return X * Other.X + Y * Other.Y + Z * Other.Z; }
  FVector3D Cross(const FVector3D& Other) const {
    return {Y * Other.Z - Z * Other.Y, Z * Other.X - X * Other.Z, X * Other.Y - Y * Other.X};
  }
  FVector3D operator+(const FVector3D& Other) const {
    return {X + Other.X, Y + Other.Y, Z + Other.Z};
  }
  FVector3D operator-(const FVector3D& Other) const {
    return {X - Other.X, Y - Other.Y, Z - Other.Z};
  }
  FVector3D operator*(const FVector3D& Other) const {
    return {X * Other.X, Y * Other.Y, Z * Other.Z};
  }
  FVector3D operator/(const FVector3D& Other) const {
    return {X / Other.X, Y / Other.Y, Z / Other.Z};
  }
  FVector3D operator*(float Value) const { return {X * Value, Y * Value, Z * Value}; }
  FVector3D operator/(float Value) const { return {X / Value, Y / Value, Z / Value}; }
  FVector3D& operator+=(const FVector3D& Other) {
    X += Other.X;
    Y += Other.Y;
    Z += Other.Z;
    return *this;
  }
  auto operator<=>(const FVector3D&) const = default;
};

struct BROCCOLI_ENGINE_API FRotator3D {
  float Pitch = 0.0f;
  float Yaw = 0.0f;
  float Roll = 0.0f;
};

struct BROCCOLI_ENGINE_API FQuaternion {
  float X = 0.0f;
  float Y = 0.0f;
  float Z = 0.0f;
  float W = 1.0f;

  static constexpr FQuaternion Identity() { return {}; }
  FQuaternion Normalize() const {
    const float Length = std::sqrt(X * X + Y * Y + Z * Z + W * W);
    return Length > 1e-6f ? FQuaternion{X / Length, Y / Length, Z / Length, W / Length}
                          : Identity();
  }
  FQuaternion Inverse() const {
    const FQuaternion Unit = Normalize();
    return {-Unit.X, -Unit.Y, -Unit.Z, Unit.W};
  }
  FQuaternion operator*(const FQuaternion& Other) const {
    return {
        W * Other.X + X * Other.W + Y * Other.Z - Z * Other.Y,
        W * Other.Y - X * Other.Z + Y * Other.W + Z * Other.X,
        W * Other.Z + X * Other.Y - Y * Other.X + Z * Other.W,
        W * Other.W - X * Other.X - Y * Other.Y - Z * Other.Z
    };
  }
  FVector3D RotateVector(const FVector3D& Vector) const {
    const FQuaternion Rotated = *this * FQuaternion{Vector.X, Vector.Y, Vector.Z, 0.0f} * Inverse();
    return {Rotated.X, Rotated.Y, Rotated.Z};
  }
  static FQuaternion FromRotator(const FRotator3D& Rotator) {
    const float PitchRadians = UMath::DegToRad(Rotator.Pitch) * 0.5f;
    const float YawRadians = UMath::DegToRad(Rotator.Yaw) * 0.5f;
    const float RollRadians = UMath::DegToRad(Rotator.Roll) * 0.5f;
    const FQuaternion Pitch{std::sin(PitchRadians), 0.0f, 0.0f, std::cos(PitchRadians)};
    const FQuaternion Yaw{0.0f, 0.0f, std::sin(YawRadians), std::cos(YawRadians)};
    const FQuaternion Roll{0.0f, std::sin(RollRadians), 0.0f, std::cos(RollRadians)};
    return (Yaw * Pitch * Roll).Normalize();
  }
  FRotator3D ToRotator() const {
    const FQuaternion Unit = Normalize();
    const float SinPitch = 2.0f * (Unit.W * Unit.X - Unit.Y * Unit.Z);
    const float Pitch = std::asin((std::clamp)(SinPitch, -1.0f, 1.0f));
    const float Yaw = std::atan2(
        2.0f * (Unit.W * Unit.Z + Unit.X * Unit.Y),
        1.0f - 2.0f * (Unit.X * Unit.X + Unit.Z * Unit.Z)
    );
    const float Roll = std::atan2(
        2.0f * (Unit.W * Unit.Y + Unit.Z * Unit.X),
        1.0f - 2.0f * (Unit.X * Unit.X + Unit.Y * Unit.Y)
    );
    return {UMath::RadToDeg(Pitch), UMath::RadToDeg(Yaw), UMath::RadToDeg(Roll)};
  }
};

struct BROCCOLI_ENGINE_API FScale3D {
  float X = 1.0f;
  float Y = 1.0f;
  float Z = 1.0f;
  FScale3D() = default;
  explicit FScale3D(float Value) : X(Value), Y(Value), Z(Value) {}
  constexpr FScale3D(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
  FScale3D operator*(const FScale3D& Other) const {
    return {X * Other.X, Y * Other.Y, Z * Other.Z};
  }
  FScale3D operator/(const FScale3D& Other) const {
    return {X / Other.X, Y / Other.Y, Z / Other.Z};
  }
  bool IsNearlyZero(float Tolerance = 1e-6f) const {
    return std::abs(X) < Tolerance || std::abs(Y) < Tolerance || std::abs(Z) < Tolerance;
  }
};

struct BROCCOLI_ENGINE_API FTransform3D {
  FVector3D Location;
  FQuaternion Rotation;
  FScale3D Scale;
  FVector3D TransformPosition(const FVector3D& Position) const {
    return Location + Rotation.RotateVector(Position * FVector3D{Scale.X, Scale.Y, Scale.Z});
  }
  FVector3D InverseTransformPosition(const FVector3D& Position) const {
    return Rotation.Inverse().RotateVector(Position - Location) /
           FVector3D{Scale.X, Scale.Y, Scale.Z};
  }
  static FTransform3D Combine(const FTransform3D& Parent, const FTransform3D& Local) {
    return {
        Parent.TransformPosition(Local.Location),
        (Parent.Rotation * Local.Rotation).Normalize(),
        Parent.Scale * Local.Scale
    };
  }
  static FTransform3D MakeRelative(const FTransform3D& World, const FTransform3D& Parent) {
    return {
        Parent.InverseTransformPosition(World.Location),
        (Parent.Rotation.Inverse() * World.Rotation).Normalize(),
        World.Scale / Parent.Scale
    };
  }
};
