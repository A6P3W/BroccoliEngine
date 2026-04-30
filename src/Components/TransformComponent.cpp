#include "Components/TransformComponent.h"
#include <cmath>
#include "Utils/UMath.h"
void MTransformComponent::SetPos(float nx, float ny)
{
 pos.x = nx;
    pos.y = ny;
}

void MTransformComponent::SetPos(const Vector2& nPos)
{
   pos = nPos;
}

void MTransformComponent::AddLocalPos(float nx, float ny)
{
    float rad = UMath::DegToRad(angle); 

    float s = std::sin(rad);
    float c = std::cos(rad);

    float worldNX = nx * c - ny * s;
    float worldNY = nx * s + ny * c;

    pos.x += worldNX;
    pos.y += worldNY;
}

void MTransformComponent::AddWorldPos(float nx, float ny)
{
	SetPos(pos.x + nx, pos.y + ny);
}

const Vector2& MTransformComponent::GetPos() const
{
   return pos;
}

void MTransformComponent::SetAngle(float nAngle)
{
    angle = nAngle;
}
void MTransformComponent::AddAngle(float nAngleDeg)
{
    angle += nAngleDeg;
}

float MTransformComponent::GetAngle() const
{
    return angle;
}

void MTransformComponent::SetScale(float nScale)
{
    scale = nScale;
}

float MTransformComponent::GetScale() const
{
    return scale;
}
