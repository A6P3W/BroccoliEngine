#pragma once

#include "PlayerController.h"

// プレイヤーの入力とアクターの制御を仲介する、エンジンのプレイヤーコントローラー基底クラス APlayerController を継承
// すべてのサンプルコントローラーに共通する「Escape キーによるメニュー復帰」処理を定義します
class ALauncherPlayerController : public APlayerController {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ALauncherPlayerController)

  // プレイヤー入力をこのコントローラーにバインドするライフサイクル関数
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void ReturnToLevelStarter();
};
