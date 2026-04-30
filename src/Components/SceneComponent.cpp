#include "Components/SceneComponent.h"
#include <cmath>
#include "Utils/UMath.h"

MSceneComponent::MSceneComponent() = default;
MSceneComponent::~MSceneComponent() = default;

void MSceneComponent::OnUpdate(float DeltaTime)
{
}

void MSceneComponent::Draw()
{
}

void MSceneComponent::OnMessage(const std::string& message)
{
}

void MSceneComponent::SetOwner(MUpdateableObject* owner)
{
    m_owner = owner;
}

void MSceneComponent::SetPos(float nx, float ny)
{
    pos.x = nx;
    pos.y = ny;
}

void MSceneComponent::SetPos(const Vector2& nPos)
{
    pos = nPos;
}

void MSceneComponent::AddLocalPos(float nx, float ny)
{
    float rad = UMath::DegToRad(angle);

    float s = std::sin(rad);
    float c = std::cos(rad);

    float worldNX = nx * c - ny * s;
    float worldNY = nx * s + ny * c;

    pos.x += worldNX;
    pos.y += worldNY;
}

void MSceneComponent::AddWorldPos(float nx, float ny)
{
    SetPos(pos.x + nx, pos.y + ny);
}

const Vector2& MSceneComponent::GetPos() const
{
    return pos;
}

void MSceneComponent::SetAngle(float nAngle)
{
    angle = nAngle;
}

void MSceneComponent::AddAngle(float nAngleDeg)
{
    angle += nAngleDeg;
}

float MSceneComponent::GetAngle() const
{
    return angle;
}

void MSceneComponent::SetScale(float nScale)
{
    scale = nScale;
}

float MSceneComponent::GetScale() const
{
    return scale;
}
