#pragma once
class UMath
{
public:
    inline static float DegToRad(float deg) {
        return deg * (3.14159265f / 180.0f);
    }
    inline static float RadToDeg (float rad) {
        return rad * (180.0f / 3.14159265f);
	}  
};

