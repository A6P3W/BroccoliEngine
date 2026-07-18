#pragma once

#include "WidgetBase.h"

class MUIButtonComponent;

// UI表示機能を持つエンジンのウィジェット基底クラス AWidgetBase を継承したウィジェットアクター
class ALevelStarterWidget : public AWidgetBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ALevelStarterWidget)

  ALevelStarterWidget();

 protected:
  // 初期化用のライフサイクル関数
  void BeginPlay() override;

 private:
  // 画面中央に配置されるボタンコンポーネントのポインタ
  MUIButtonComponent* OpenButton = nullptr;
};
