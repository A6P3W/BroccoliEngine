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

	float rad = UMath::DegToRad(GetWorldRotation().Rotation);
	float cosA = std::cos(rad);
	float sinA = std::sin(rad);

	// 回転を考慮した4つのローカル頂点
	FVector2D corners[4] = {
		{ -halfWidth, -halfHeight },
		{  halfWidth, -halfHeight },
		{ -halfWidth,  halfHeight },
		{  halfWidth,  halfHeight }
	};

	FAABB aabb{ 1e9f, 1e9f, -1e9f, -1e9f };
	for (int i = 0; i < 4; ++i) {
		float rotatedX = corners[i].X * cosA - corners[i].Y * sinA;
		float rotatedY = corners[i].X * sinA + corners[i].Y * cosA;
		float worldX = center.X + rotatedX;
		float worldY = center.Y + rotatedY;

		if (worldX < aabb.MinX) aabb.MinX = worldX;
		if (worldY < aabb.MinY) aabb.MinY = worldY;
		if (worldX > aabb.MaxX) aabb.MaxX = worldX;
		if (worldY > aabb.MaxY) aabb.MaxY = worldY;
	}
	return aabb;
}
