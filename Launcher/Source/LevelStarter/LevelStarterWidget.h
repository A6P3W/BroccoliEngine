#pragma once

#include <nlohmann/json.hpp>

#include "AutomationAnnotations.h"
#include "WidgetBase.h"

class MUIButtonComponent;

// UI表示機能を持つエンジンのウィジェット基底クラス AWidgetBase を継承したウィジェットアクター
class ALevelStarterWidget : public AWidgetBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ALevelStarterWidget)

  ALevelStarterWidget();

  CONTROL_METHOD(
          .Name = "get_status",
          .Description = "Return the LevelStarter widget status for automation verification."
  )
  nlohmann::json GetAutomationStatus() const;

 protected:
  // 初期化用のライフサイクル関数
  void BeginPlay() override;

 private:
  // 画面中央に配置されるボタンコンポーネントのポインタ
  MUIButtonComponent* OpenButton = nullptr;
};
