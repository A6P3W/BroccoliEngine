#pragma once
#include "WidgetBase.h"
// 各種 UI コントロール（ボタン、トグル、テキスト入力等）の配置・制御を行うメイン UI ウィジェットアクター
class AWidgetTestUIMain : public AWidgetBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(AWidgetTestUIMain)
  AWidgetTestUIMain();
};
