#include "LevelStarterWidget.h"

#include <DxLib.h>

#include <memory>
#include <string>

#include "FileDialog.h"
#include "SceneManager.h"
#include "UIBoxButton.h"
#include "UIManager.h"
#include "UITextComponent.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ALevelStarterWidget)

ALevelStarterWidget::ALevelStarterWidget() {
  constexpr float ButtonWidth = 300.0f;
  constexpr float ButtonHeight = 60.0f;

  // 指定したアクター（this）を所有者として、ボタン用の矩形 UI コンポーネント UIBoxButtonComponent を動的に作成するエンジンの関数
  auto* buttonPtr = NewObject<UIBoxButtonComponent>(this);
  // ボタンのサイズ（幅・高さ）を設定
  buttonPtr->SetSize(ButtonWidth, ButtonHeight);
  // 通常時、ホバー時、クリック（押下）時のボタンカラーをそれぞれ設定
  buttonPtr->SetColors(FColor{85, 85, 85}, FColor{119, 119, 119}, FColor{51, 51, 51});
  // ボタンを画面中央（MiddleCenter）に配置するアンカー設定
  buttonPtr->SetAnchor(EUIAnchor::MiddleCenter);
  // ボタンが押された（クリックされた）際に実行するコールバックをラムダ式で登録
  buttonPtr->SetOnPressed([]() {
    // OS のファイル選択ダイアログを表示して、.BLevel ファイルのパスを取得
    const std::string filepath =
        FileDialog::OpenFile("Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0");
    if (!filepath.empty()) {
      // 取得したパスのレベル（ステージ）をロードする
      SceneManager::GetInstance().OpenLevelByPath(filepath);
    }
  });

  // テキストを描画する UITextComponent を動的に作成
  auto* text = NewObject<UITextComponent>(this);
  // 表示するテキスト文字列を設定
  text->SetText("Select .BLevel to Play");
  // 文字色を白色に設定
  text->SetColor(FColor{255, 255, 255});
  // フォントサイズを 24 に設定
  text->SetFontSize(24);
  // このテキストをボタンコンポーネントにアタッチ（ボタンと親子関係を構築）
  text->AttachToComponent(buttonPtr);
  // アタッチ先（ボタン）の中央（MiddleCenter）にテキストを配置するよう設定
  text->SetAnchor(EUIAnchor::MiddleCenter);

  OpenButton = buttonPtr;

  // 各コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  buttonPtr->RegisterComponent();
  text->RegisterComponent();
}

void ALevelStarterWidget::BeginPlay() {
  // 基底クラス AWidgetBase の BeginPlay 処理を実行
  AWidgetBase::BeginPlay();

  // UI 管理用のシングルトンマネージャー UIManager を取得
  if (auto* uiManager = UIManager::GetInstance()) {
    // このウィジェットアクターを UI 表示リストに追加
    uiManager->AddWidget(this);
    // このウィジェットに入力フォーカスを設定
    uiManager->SetFocusedWidget(this);
  }

  // 作成したボタン（OpenButton）をゲームパッドやキーボード選択のデフォルトフォーカスに設定
  SetFocusedButton(OpenButton);
}
