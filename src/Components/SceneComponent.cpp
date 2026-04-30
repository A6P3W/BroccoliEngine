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

void MSceneComponent::SetLocation(const FVector2D& nPos)
{
	Location = nPos;
}

void MSceneComponent::AddLocalLocation(float nx, float ny)
{
	float rad = UMath::DegToRad(Rotation.Rotation);

	float s = std::sin(rad);
	float c = std::cos(rad);

	float worldNX = nx * c - ny * s;
	float worldNY = nx * s + ny * c;

	Location.X += worldNX;
	Location.Y += worldNY;
}

void MSceneComponent::AddWorldLocation(float nx, float ny)
{
	SetLocation({Location.X + nx, Location.Y + ny});
}

const FVector2D& MSceneComponent::GetLocation() const
{
	return Location;
}

void MSceneComponent::SetRotation(float nAngle)
{
	Rotation = nAngle;
}

void MSceneComponent::AddRotation(float nAngleDeg)
{
	Rotation.Rotation += nAngleDeg;
}

float MSceneComponent::GetRotation() const
{
	return Rotation.Rotation;
}

void MSceneComponent::SetScale(float nScale)
{
	Scale = nScale;
}

float MSceneComponent::GetScale() const
{
	return Scale;
}
