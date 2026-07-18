#pragma once
#include "GameModeBase.h"
// UIコントロールのナビゲーション検証用のゲームモードクラス AGameModeBase を継承
class AWidgetTestGameMode : public AGameModeBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(AWidgetTestGameMode)
  AWidgetTestGameMode();
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
};
