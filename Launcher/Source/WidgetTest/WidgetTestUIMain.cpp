#include "WidgetTestUIMain.h"

#include <DxLib.h>

#include "Application.h"
#include "Log.h"
#include "UIBoxButton.h"
#include "UIInputTextComponent.h"
#include "UITextComponent.h"
#include "UIToggleButtonComponent.h"
#include "UIVerticalBoxComponent.h"

AWidgetTestUIMain::AWidgetTestUIMain() {
  float btnWidth = 200.0f;
  float btnHeight = 50.0f;
  float sideBtnWidth = 120.0f;
  float sideBtnHeight = 50.0f;
  FColor normalColor = FColor{100, 100, 100};
  FColor hoveredColor = FColor{150, 150, 150};
  FColor pressedColor = FColor{50, 50, 50};
  FColor inputEditingColor = FColor{40, 100, 170};

  // UI要素を縦方向に並べて配置を管理するコンポーネント MUIVerticalBoxComponent を作成
  auto* VerticalBox = NewObject<MUIVerticalBoxComponent>(this);
  auto* VerticalBoxPtr = VerticalBox;
  // 垂直ボックスの画面アタッチアンカーを中央（MiddleCenter）に設定
  VerticalBoxPtr->SetAnchor(EUIAnchor::MiddleCenter);
  // 位置基準（ピボット）をボックスの中心（0.5f, 0.5f）に設定
  VerticalBoxPtr->SetPivot({0.5f, 0.5f});
  // 垂直ボックスの幅をボタン幅に設定
  VerticalBoxPtr->SetWidgetSize({btnWidth, 0.0f});
  // 項目間のピクセル間隔を 24.0f に設定
  VerticalBoxPtr->SetSpacing(24.0f);
  // 内包する要素の数に合わせて、ボックスの縦サイズを自動伸縮させるよう設定
  VerticalBoxPtr->SetAutoResize(true);
  // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  VerticalBox->RegisterComponent();

  // 指定したアクター（this）を所有者として、ボタン用の矩形 UI コンポーネント（UIBoxButtonComponent）を動的に作成するエンジンの関数
  auto* StartBtn = NewObject<UIBoxButtonComponent>(this);
  auto* StartBtnPtr = StartBtn;
  StartBtnPtr->SetSize(btnWidth, btnHeight);
  StartBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  // 垂直ボックスへボタン要素を追加
  VerticalBoxPtr->AddItem(StartBtnPtr);
  StartBtn->RegisterComponent();

  // Toggleボタン
  auto* ToggleBtn = NewObject<UIToggleButtonComponent>(this);
  auto* ToggleBtnPtr = ToggleBtn;
  ToggleBtnPtr->SetSize(btnWidth, btnHeight);
  // トグルボタンの状態に応じたカラー群を設定
  ToggleBtnPtr->SetColors(
      FColor{46, 160, 67},
      FColor{61, 184, 82},
      FColor{35, 134, 54},
      FColor{90, 90, 90},
      FColor{125, 125, 125},
      FColor{60, 60, 60}
  );
  // 垂直ボックスへトグルボタン要素を追加
  VerticalBoxPtr->AddItem(ToggleBtnPtr);
  ToggleBtn->RegisterComponent();

  // Configボタン
  auto* ConfigBtn = NewObject<UIBoxButtonComponent>(this);
  auto* ConfigBtnPtr = ConfigBtn;
  ConfigBtnPtr->SetSize(btnWidth, btnHeight);
  ConfigBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  // 垂直ボックスへ追加
  VerticalBoxPtr->AddItem(ConfigBtnPtr);
  ConfigBtn->RegisterComponent();

  // 入力可能テキスト欄
  auto* NameInput = NewObject<UIInputTextComponent>(this);
  auto* NameInputPtr = NameInput;
  NameInputPtr->SetSize(btnWidth, btnHeight);
  // 何も入力されていないときに薄く表示するヒントテキストを設定
  NameInputPtr->SetHintText("Name...");
  // 最大入力文字数を 12 文字に制限
  NameInputPtr->SetMaxLength(12);
  NameInputPtr->SetColors(normalColor, hoveredColor, inputEditingColor);
  // 垂直ボックスへ追加
  VerticalBoxPtr->AddItem(NameInputPtr);
  NameInput->RegisterComponent();

  // Exitボタン
  auto* ExitBtn = NewObject<UIBoxButtonComponent>(this);
  auto* ExitBtnPtr = ExitBtn;
  ExitBtnPtr->SetSize(btnWidth, btnHeight);
  ExitBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  // 垂直ボックスへ追加
  VerticalBoxPtr->AddItem(ExitBtnPtr);
  ExitBtn->RegisterComponent();

  // 画面左側に配置するナビゲーション用ボタン
  auto* LeftNavBtn = NewObject<UIBoxButtonComponent>(this);
  auto* LeftNavBtnPtr = LeftNavBtn;
  LeftNavBtnPtr->SetSize(sideBtnWidth, sideBtnHeight);
  LeftNavBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  // アンカー基準からの絶対座標を指定して配置
  LeftNavBtnPtr->SetAnchoredPosition({-260.0f, 0.0f});
  LeftNavBtn->RegisterComponent();

  // 画面右側に配置するナビゲーション用ボタン
  auto* RightNavBtn = NewObject<UIBoxButtonComponent>(this);
  auto* RightNavBtnPtr = RightNavBtn;
  RightNavBtnPtr->SetSize(sideBtnWidth, sideBtnHeight);
  RightNavBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  // アンカー基準からの絶対座標を指定して配置
  RightNavBtnPtr->SetAnchoredPosition({260.0f, 0.0f});
  RightNavBtn->RegisterComponent();

  auto* startText = NewObject<UITextComponent>(this);
  startText->SetText("Start");
  startText->SetColor(FColor{255, 255, 255});
  startText->SetFontSize(24);
  startText->AttachToComponent(StartBtnPtr);  // ボタンの子要素にするだけで中央に配置される
  startText->RegisterComponent();

  auto* toggleText = NewObject<UITextComponent>(this);
  auto* ToggleTextPtr = toggleText;
  ToggleTextPtr->SetText("Toggle: OFF");
  ToggleTextPtr->SetColor(FColor{255, 255, 255});
  ToggleTextPtr->SetFontSize(24);
  ToggleTextPtr->AttachToComponent(ToggleBtnPtr);
  toggleText->RegisterComponent();

  auto* leftNavText = NewObject<UITextComponent>(this);
  leftNavText->SetText("Left");
  leftNavText->SetColor(FColor{255, 255, 255});
  leftNavText->SetFontSize(24);
  leftNavText->AttachToComponent(LeftNavBtnPtr);
  leftNavText->RegisterComponent();

  auto* rightNavText = NewObject<UITextComponent>(this);
  rightNavText->SetText("Right");
  rightNavText->SetColor(FColor{255, 255, 255});
  rightNavText->SetFontSize(24);
  rightNavText->AttachToComponent(RightNavBtnPtr);
  rightNavText->RegisterComponent();

  auto* committedText = NewObject<UITextComponent>(this);
  auto* CommittedTextPtr = committedText;
  CommittedTextPtr->SetAnchor(EUIAnchor::MiddleCenter);
  CommittedTextPtr->SetAnchoredPosition({0.0f, 210.0f});
  committedText->SetText("Input: <empty>");
  committedText->SetColor(FColor{210, 230, 255});
  committedText->SetFontSize(20);
  committedText->RegisterComponent();

  // 各種UIの押下・値変更時のイベントハンドリングを設定
  StartBtnPtr->SetOnPressed([]() { M_LOG("Start Game!"); });

  // トグルボタンの状態変更イベント
  ToggleBtnPtr->SetOnToggled([ToggleTextPtr](bool bIsOn) {
    ToggleTextPtr->SetText(bIsOn ? "Toggle: ON" : "Toggle: OFF");
    M_LOG("Toggle Button: {}", bIsOn ? "ON" : "OFF");
  });

  // テキストボックスの編集中の文字列変更イベント
  NameInputPtr->SetOnTextChanged([](const std::string& text) { M_LOG("Input changed: {}", text); });

  // テキストボックスの編集完了（Enter押下など）イベント
  NameInputPtr->SetOnTextCommitted([CommittedTextPtr](const std::string& text) {
    const std::string displayText = text.empty() ? "<empty>" : text;
    CommittedTextPtr->SetText("Input: " + displayText);
    M_LOG("Input committed: {}", displayText);
  });

  // ゲーム終了処理のイベント
  ExitBtnPtr->SetOnPressed([]() {
    M_LOG("Exit Game!");
    // アプリケーションを終了する
    Application::QuitGame();
  });
  LeftNavBtnPtr->SetOnPressed([]() { M_LOG("Left navigation button pressed."); });
  RightNavBtnPtr->SetOnPressed([]() { M_LOG("Right navigation button pressed."); });

  // 垂直ボックス内の要素間で、上下の方向キー操作によるナビゲーション（フォーカス遷移）を自動構築
  VerticalBoxPtr->BuildNavigation();
  // 垂直ボックス内の要素を選択中に「左」キーを押した際の移動先を設定
  VerticalBoxPtr->SetNavigationLeft(LeftNavBtnPtr);
  // 垂直ボックス内の要素を選択中に「右」キーを押した際の移動先を設定
  VerticalBoxPtr->SetNavigationRight(RightNavBtnPtr);
  // 個別ボタンの左右ナビゲーション移動先を設定
  LeftNavBtnPtr->Navigation.Right = StartBtnPtr;
  RightNavBtnPtr->Navigation.Left = StartBtnPtr;

  // UI を開いたときに最初にフォーカスを当てるパーツを設定
  SetFocusedButton(StartBtnPtr);
}
