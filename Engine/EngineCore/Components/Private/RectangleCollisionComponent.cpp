#include "RectangleCollisionComponent.h"
#include <RenderSystem.h>
#include "EngineDefine.h"

void MRectangleCollisionComponent::Draw()
{
	if (!IsDebug)return;
	FVector2D center = GetWorldLocation();
	float halfWidth = (m_width * GetWorldScale()) * 0.5f;
	float halfHeight = (m_height * GetWorldScale()) * 0.5f;

	// 回転時のピボット(左上)を計算するために、中心から左上へのベクトルを回転させます
	float rad = UMath::DegToRad(GetWorldRotation().Rotation);
	float cosA = std::cos(rad);
	float sinA = std::sin(rad);

	FVector2D rotatedTopLeftOffset = {
		-halfWidth * cosA - (-halfHeight) * sinA,
		-halfWidth * sinA + (-halfHeight) * cosA
	};

	RenderSystem::GetInstance().SubmitBox(
		{ center.X + rotatedTopLeftOffset.X, center.Y + rotatedTopLeftOffset.Y }, // 回転を考慮した左上座標
		{ halfWidth * 2.0f, halfHeight * 2.0f },
		GetWorldRotation().Rotation,
		0x00FF00,
		0,
		RenderSpace::World,
		100,
		120);
}

FAABB MRectangleCollisionComponent::GetAABB() const
{
	FVector2D center = GetWorldLocation();
	float halfWidth = (m_width * GetWorldScale()) * 0.5f;
	float halfHeight = (m_height * GetWorldScale()) * 0.5f;

	FAABB aabb;
	aabb.MinX = center.X - halfWidth;
	aabb.MinY = center.Y - halfHeight;
	aabb.MaxX = center.X + halfWidth;
	aabb.MaxY = center.Y + halfHeight;
	return aabb;
}
