#include "Objects/SampleA.h"
#include "Dxlib.h"
#include "Systems/ResourceManager.h"
#include "Components/SpriteComponent.h" 

SampleA::SampleA(float x, float y) : OGameObject() // 親のコンストラクタを呼ぶ
{
	m_transform->SetPos(x, y); // TransformComponent を使って位置を設定
	// 1. リソースマネージャーから画像ハンドルを取得
	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

	// 2. SpriteComponent を作成して追加
    auto sprite = std::make_unique<SpriteComponent>(handle, 0);
	AddComponent(std::move(sprite));
}

SampleA::~SampleA()
{
}

void SampleA::OnUpdate()
{
	m_transform->AddAngle(0.05f); // 毎フレーム回転させる
	m_transform->AddLocalPos(2.0f, 0.0f); // 毎フレーム右に移動させる
}

void SampleA::OnDraw()
{
}