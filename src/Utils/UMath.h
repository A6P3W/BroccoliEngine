#pragma once
#include <cmath>
#include <numbers>
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

