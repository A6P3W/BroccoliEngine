#pragma once
#include <cmath>
#include <numbers>
struct FVector2D
{
    float X = 0.0f;
    float Y = 0.0f;

    static const FVector2D ZeroVector;
};

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
    inline static float RadToDeg (float rad) {
        return rad * (180.0f / std::numbers::pi_v<float>);
	}  
};

