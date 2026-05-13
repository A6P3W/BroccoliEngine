#pragma once
#include <cmath>
#include <numbers>
struct FScale
{
	float Scale = 1.0f;
	FScale() = default;
	FScale(float InScale) : Scale(InScale) {}
};
struct FVector2D {
	float X = 0.0f;
	float Y = 0.0f;

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
};
inline FVector2D operator*(FVector2D Vec, float v) {
	Vec *= v;
	return Vec;
}
inline const FVector2D FVector2D::ZeroVector{ 0.0f, 0.0f };

struct FRotator
{
	//Degrees
	float Rotation = 0.0f;

	FRotator() = default;
	FRotator(float InRotation) : Rotation(InRotation) {}
};
class UMath
{
public:
	inline static float DegToRad(float deg) {
		return deg * (std::numbers::pi_v<float> / 180.0f);
	}
	inline static float RadToDeg(float rad) {
		return rad * (180.0f / std::numbers::pi_v<float>);
	}
};

