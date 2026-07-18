#pragma once

#include "GameModeBase.h"

// ゲーム全体のルールや初期スポーン設定を管理する、エンジンのゲームモード基底クラスAGameModeBaseを継承
class ABasicGameplayGameMode : public AGameModeBase {
 public:
  // クラスのメタデータ（型名 StaticClassName(),
  // クラス名取得GetActorClassName()など）を定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ABasicGameplayGameMode)

  ABasicGameplayGameMode();
};
