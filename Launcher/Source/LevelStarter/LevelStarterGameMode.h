#pragma once

#include "GameModeBase.h"

// ゲーム全体の基本ルールや初期スポーン設定を管理するエンジンのゲームモード基底クラス AGameModeBase を継承
class ALevelStarterGameMode : public AGameModeBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ALevelStarterGameMode)

  ALevelStarterGameMode();

 protected:
  // ゲーム開始時またはアクター生成時に呼び出される初期化用ライフサイクル関数
  void BeginPlay() override;
};