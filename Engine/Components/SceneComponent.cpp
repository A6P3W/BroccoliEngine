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

void MSceneComponent::SetParentComponent(MSceneComponent* parent)
{
	if (!parent) { return; }
	m_parentComponent = parent;
	parent->m_childComponents.push_back(this);
}

bool MSceneComponent::SetWorldLocation(const FVector2D& NewWorldLocation)
{
	if (!m_parentComponent) {
		// 親がいない（RootComponentである）場合、
		// ワールド座標はそのまま相対座標（RelativeLocation）になる
		RelativeLocation = NewWorldLocation;
	}
	else {
		// 親がいる場合、ワールド座標を親のローカル座標系に変換する必要がある
		FVector2D ParentWorldLoc = m_parentComponent->GetWorldLocation();
		float ParentWorldRotRad = UMath::DegToRad(m_parentComponent->GetWorldRotation().Rotation);

		// 1. 親との差分を取る
		float diffX = NewWorldLocation.X - ParentWorldLoc.X;
		float diffY = NewWorldLocation.Y - ParentWorldLoc.Y;

		// 2. 親の回転の「逆」をかけて、親から見た相対位置を出す
		// 逆回転行列: [ cos(-θ) -sin(-θ) ] = [  cosθ  sinθ ]
		//              [ sin(-θ)  cos(-θ) ]   [ -sinθ  cosθ ]
		float s = std::sin(ParentWorldRotRad);
		float c = std::cos(ParentWorldRotRad);

		RelativeLocation.X = diffX * c + diffY * s;
		RelativeLocation.Y = -diffX * s + diffY * c;
	}
	return true;
}

bool MSceneComponent::SetRelativeLocation(const FVector2D& NewRelativeLocation)
{
	RelativeLocation = NewRelativeLocation;
	return true;
}

bool MSceneComponent::AddWorldOffset(const FVector2D& Offset)
{
   FVector2D newWorldLocation = GetWorldLocation() + Offset;
	return SetWorldLocation(newWorldLocation);
}

bool MSceneComponent::AddLocalOffset(const FVector2D& Offset)
{
	float worldRotRad = UMath::DegToRad(GetWorldRotation().Rotation);
	float s = std::sin(worldRotRad);
	float c = std::cos(worldRotRad);

	FVector2D worldOffset;
	worldOffset.X = Offset.X * c - Offset.Y * s;
	worldOffset.Y = Offset.X * s + Offset.Y * c;

	FVector2D newWorldLocation = GetWorldLocation() + worldOffset;
	return SetWorldLocation(newWorldLocation);
}



bool MSceneComponent::SetWorldRotation(float nAngle)
{
	if (!m_parentComponent) {
		RelativeRotation.Rotation = nAngle;

	}
	else {
		float parentWorldRot = m_parentComponent->GetWorldRotation().Rotation;
		RelativeRotation.Rotation = nAngle - parentWorldRot;
	}
	return true;
}

bool MSceneComponent::AddWorldRotation(float nAngleDeg)
{
	RelativeRotation.Rotation += nAngleDeg;
	return true;
}


FVector2D MSceneComponent::GetWorldLocation() const
{
	if (!m_parentComponent) {
		return RelativeLocation;
	}

	// 親のワールド回転角を取得
	float parentRad = UMath::DegToRad(m_parentComponent->GetWorldRotation().Rotation);
	float s = std::sin(parentRad);
	float c = std::cos(parentRad);

	// 親の向きに合わせて、自分の相対座標を回転させてオフセットを計算
	float rotatedX = RelativeLocation.X * c - RelativeLocation.Y * s;
	float rotatedY = RelativeLocation.X * s + RelativeLocation.Y * c;

	FVector2D ParentWorldLoc = m_parentComponent->GetWorldLocation();
	return { ParentWorldLoc.X + rotatedX, ParentWorldLoc.Y + rotatedY };
}

FVector2D MSceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}

FRotator MSceneComponent::GetWorldRotation() const
{
	if (!m_parentComponent) {
		return RelativeRotation;
	}
	return FRotator(m_parentComponent->GetWorldRotation().Rotation + RelativeRotation.Rotation);
}

FRotator MSceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}

bool MSceneComponent::SetScale(float nScale)
{
	Scale = nScale;
	return true;
}

float MSceneComponent::GetScale() const
{
 if (!m_parentComponent) {
		return Scale.Scale;
	}
	return Scale.Scale * m_parentComponent->GetScale();
}
