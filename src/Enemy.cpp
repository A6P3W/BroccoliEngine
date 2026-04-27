#include "Enemy.h"
#include "Dxlib.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "SpriteComponent.h" 

Enemy::Enemy(float x, float y) : GameObject() // 親のコンストラクタを呼ぶ
{
    auto transform = std::make_unique<TransformComponent>();
    m_transform = transform.get();
    m_transform->SetPos(x, y);
    AddComponent(std::move(transform));
    // 1. リソースマネージャーから画像ハンドルを取得
    int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

    // 2. SpriteComponent を作成して追加
    auto sprite = std::make_unique<SpriteComponent>(handle);
    AddComponent(std::move(sprite));
}

Enemy::~Enemy()
{
}

void Enemy::OnUpdate()
{
    if (m_transform) {
        float x =m_transform->GetX();
        m_transform->SetPos(x+2, 255);
    }
}

void Enemy::OnDraw()
{
}