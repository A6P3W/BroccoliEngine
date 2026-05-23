#include "Components/Public/SceneComponent.h"
#include <cmath>
#include "Utils/UMath.h"
#include "Actor.h"

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
	MakeTransformDirty();
	return true;
}

bool MSceneComponent::SetRelativeLocation(const FVector2D& NewRelativeLocation)
{
	RelativeLocation = NewRelativeLocation;
	MakeTransformDirty();
	return true;
}

bool MSceneComponent::AddWorldOffset(const FVector2D& Offset)
{
	FVector2D newWorldLocation = GetWorldLocation() + Offset;
	MakeTransformDirty();
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
	MakeTransformDirty();
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
	MakeTransformDirty();
	return true;
}

bool MSceneComponent::AddWorldRotation(float nAngleDeg)
{
	RelativeRotation.Rotation += nAngleDeg;
	MakeTransformDirty();
	return true;
}


FVector2D MSceneComponent::GetWorldLocation() const
{
	UpdateTransform();
	return WorldLocation;
}

FVector2D MSceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}

FRotator MSceneComponent::GetWorldRotation() const
{
	UpdateTransform();
	return WorldRotation;
}

FRotator MSceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}

bool MSceneComponent::SetScale(float nScale)
{
	Scale = nScale;
	MakeTransformDirty();
	return true;
}

float MSceneComponent::GetScale() const
{
	UpdateTransform();
	return WorldScale.Scale;
}

void MSceneComponent::SetVisibility(bool bNewVisibility)
{
	bVisible = bNewVisibility;
	for (MSceneComponent* child : m_childComponents) {
		child->SetVisibility(bNewVisibility);
	}
}

void MSceneComponent::MakeTransformDirty()
{
	if (IsTransformDirty) { return; }
	IsTransformDirty = true;
	IsGridDirty = true;
	for (MSceneComponent* child : m_childComponents) {
		child->MakeTransformDirty();
	}
}

void MSceneComponent::UpdateTransform() const
{
	if (!IsTransformDirty) return;

	if (!m_parentComponent) {
		// 親がいない場合は相対値がそのままワールド値
		WorldLocation = RelativeLocation;
		WorldRotation = RelativeRotation;
		WorldScale = Scale.Scale;
	}
	else {
		// 親の最新トランスフォームを先に確定させる（再帰）
		float pScale = m_parentComponent->GetScale();
		FRotator pRot = m_parentComponent->GetWorldRotation();
		FVector2D pLoc = m_parentComponent->GetWorldLocation();

		// 1. スケールの計算
		WorldScale = Scale.Scale * pScale;

		// 2. 回転の計算
		WorldRotation.Rotation = pRot.Rotation + RelativeRotation.Rotation;

		// 3. 座標の計算（親の回転とスケールを考慮）
		float parentRad = UMath::DegToRad(pRot.Rotation);
		float s = std::sin(parentRad);
		float c = std::cos(parentRad);

		// 自分の相対座標に親のスケールを適用
		float scaledX = RelativeLocation.X * pScale;
		float scaledY = RelativeLocation.Y * pScale;

		// 親の回転に合わせて回転させる
		float rotatedX = scaledX * c - scaledY * s;
		float rotatedY = scaledX * s + scaledY * c;

		WorldLocation.X = pLoc.X + rotatedX;
		WorldLocation.Y = pLoc.Y + rotatedY;
	}

	IsTransformDirty = false;
}
