#include "StressTestActor.h"

#include "RectangleCollisionComponent.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(AStressTestActor)

AStressTestActor::AStressTestActor() {
  int texHandle =
      ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

  const int rows = 10;
  const int cols = 10;
  const float size = 64.0f;

  // アクタの中心（RootComponent）を基準に中心揃えで配置するためのオフセット計算
  float offsetX = (cols - 1) * size * 0.5f;
  float offsetY = (rows - 1) * size * 0.5f;

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      float posX = x * size - offsetX;
      float posY = y * size - offsetY;

      auto sprite = std::make_unique<MSpriteComponent>();
      sprite->SubmitGraph(texHandle);
      sprite->SetRelativeLocation({posX, posY});
      sprite->SetWorldScale(FScale(0.9f));
      AddComponent(std::move(sprite));

      auto collision = std::make_unique<MRectangleCollisionComponent>(size, size);
      collision->SetStatic(true);
      collision->SetRelativeLocation({posX, posY});
      AddComponent(std::move(collision));
    }
  }
}
