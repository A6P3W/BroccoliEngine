#pragma once
#include <cmath>
#include <numbers>
#include <compare>

class UMath {
public:
	inline static float DegToRad(float deg) {
		return deg * (std::numbers::pi_v<float> / 180.0f);
	}
	inline static float RadToDeg(float rad) {
		return rad * (180.0f / std::numbers::pi_v<float>);
	}
};

struct FRotator;

struct FScale {
	float Scale = 1.0f;
	FScale() = default;
	FScale(float InScale) : Scale(InScale) {}
};

struct FVector2D {
	float X = 0.0f;
	float Y = 0.0f;

	float SizeSquared() const {
		return X * X + Y * Y;
	}

	static const FVector2D ZeroVector;

	FVector2D& operator*=(float v) {
		X *= v;
		Y *= v;
		return *this;
	}
	FVector2D operator+(float v) const {
		return { X + v,Y + v };
	}

	FVector2D operator+(const FVector2D& v) const {
		return { X + v.X, Y + v.Y };
	}
	FVector2D RotateVector(const FRotator& Angle) const;
};

struct FRotator {
	float Rotation = 0.0f;

	FRotator() = default;
	FRotator(float InRotation) : Rotation(InRotation) {}

	// 加算演算子
	FRotator operator+(const FRotator& Other) const {
		return FRotator(Rotation + Other.Rotation);
	}

	// 減算演算子
	FRotator operator-(const FRotator& Other) const {
		return FRotator(Rotation - Other.Rotation);
	}

	// 複合代入演算子
	FRotator& operator+=(const FRotator& Other) {
		Rotation += Other.Rotation;
		return *this;
	}

	FRotator& operator-=(const FRotator& Other) {
		Rotation -= Other.Rotation;
		return *this;
	}

	// 比較演算子
	auto operator<=>(const FRotator&) const = default;
};

inline FVector2D FVector2D::RotateVector(const FRotator& Angle) const {
	float Rad = UMath::DegToRad(Angle.Rotation);
	float CosTheta = std::cos(Rad);
	float SinTheta = std::sin(Rad);

	return {
		X * CosTheta - Y * SinTheta,
		X * SinTheta + Y * CosTheta
	};
}

inline FVector2D operator*(FVector2D Vec, float v) {
	Vec *= v;
	return Vec;
}

inline const FVector2D FVector2D::ZeroVector{ 0.0f, 0.0f };
