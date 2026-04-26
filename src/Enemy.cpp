#include "Enemy.h"
#include "Dxlib.h"
#include "ResourceManager.h"
#include "SpriteComponent.h" // 追加

Enemy::Enemy(float x, float y) : GameObject(x, y) // 親のコンストラクタを呼ぶ
{
    // 1. リソースマネージャーから画像ハンドルを取得
    int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

    // 2. SpriteComponent を作成して追加
    auto sprite = std::make_unique<SpriteComponent>(handle);
    AddComponent(std::move(sprite));
}

Enemy::~Enemy()
{
}

bool Enemy::Update()
{
    // 親クラスの Update を呼ぶことでコンポーネントの Update が実行される
    GameObject::Update();
    SetPos(2,255);
    // Enemy 固有の移動ロジック（例：左に動く）
    // SetPos(GetX() - 1.0f, GetY()); 

    return true;
}

bool Enemy::Draw()
{
    // 親クラスの Draw を呼ぶことで SpriteComponent の Draw が実行される
    GameObject::Draw();
    return true;
}