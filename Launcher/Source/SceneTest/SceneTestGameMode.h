#pragma once

#include "GameModeBase.h"

// シーン階層やアタッチの検証で使用するゲームモード基底クラス AGameModeBase を継承
class ASceneTestGameMode : public AGameModeBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ASceneTestGameMode)
  ASceneTestGameMode();
};
