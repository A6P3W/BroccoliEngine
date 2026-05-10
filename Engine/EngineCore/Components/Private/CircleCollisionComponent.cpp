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
