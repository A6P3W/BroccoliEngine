#include "CircleCollisionComponent.h"
#include <RenderSystem.h>
void MCircleCollisionComponent::Draw()
{
	// デバッグ用として緑色の円を描画（透過度120程度）
	// 塗りつぶさない設定(fill=0)で枠線のみ表示
	RenderSystem::GetInstance().SubmitCircle(
		GetWorldLocation().X,
		GetWorldLocation().Y,
		m_radius * GetScale(), // ワールドスケールを考慮
		0x00FF00,             // 緑色
		0,                    // 塗りつぶしなし
		RenderSpace::World,
		100,                  // 描画優先度（手前に表示）
		120                   // アルファ値
	);
}

FAABB MCircleCollisionComponent::GetAABB() const
{
	FVector2D center = GetWorldLocation();
	float radius = m_radius * GetScale();

	FAABB aabb;
	aabb.MinX = center.X - radius;
	aabb.MinY = center.Y - radius;
	aabb.MaxX = center.X + radius;
	aabb.MaxY = center.Y + radius;
	return aabb;
}
