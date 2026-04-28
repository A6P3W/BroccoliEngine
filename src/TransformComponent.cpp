#include "TransformComponent.h"
#include <cmath>
#include "UMath.h"
void TransformComponent::SetPos(float nx, float ny)
{
 pos.x = nx;
    pos.y = ny;
}

void TransformComponent::SetPos(const Vector2& nPos)
{
   pos = nPos;
}

void TransformComponent::AddLocalPos(float nx, float ny)
{
    float rad = UMath::DegToRad(angle); 

    float s = std::sin(rad);
    float c = std::cos(rad);

    float worldNX = nx * c - ny * s;
    float worldNY = nx * s + ny * c;

    pos.x += worldNX;
    pos.y += worldNY;
}

void TransformComponent::AddWorldPos(float nx, float ny)
{
	SetPos(pos.x + nx, pos.y + ny);
}

const Vector2& TransformComponent::GetPos() const
{
   return pos;
}

void TransformComponent::SetAngle(float nAngle)
{
    angle = nAngle;
}
void TransformComponent::AddAngle(float nAngleDeg)
{
    angle += nAngleDeg;
}

float TransformComponent::GetAngle() const
{
    return angle;
}

void TransformComponent::SetScale(float nScale)
{
    scale = nScale;
}

float TransformComponent::GetScale() const
{
    return scale;
}
