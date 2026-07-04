#include "WidgetTestUIMain.h"

#include <DxLib.h>

#include "Application.h"
#include "Log.h"
#include "UIBoxButton.h"
#include "UITextComponent.h"
#include "UIToggleButtonComponent.h"
AWidgetTestUIMain::AWidgetTestUIMain() {
  float btnWidth = 200.0f;
  float btnHeight = 50.0f;
  int normalColor = GetColor(100, 100, 100);
  int hoveredColor = GetColor(150, 150, 150);
  int pressedColor = GetColor(50, 50, 50);

  // Startボタン
  auto StartBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto StartBtnPtr = StartBtn.get();
  StartBtnPtr->SetAnchoredPosition({0.0f, -240.0f});
  AddComponent(std::move(StartBtn));

  // Toggleボタン
  auto ToggleBtn = std::make_unique<UIToggleButtonComponent>(
      btnWidth, btnHeight, GetColor(46, 160, 67), GetColor(90, 90, 90)
  );
  auto ToggleBtnPtr = ToggleBtn.get();
  ToggleBtnPtr->SetAnchoredPosition({0.0f, -80.0f});
  ToggleBtnPtr->SetColors(
      GetColor(46, 160, 67),
      GetColor(61, 184, 82),
      GetColor(35, 134, 54),
      GetColor(90, 90, 90),
      GetColor(125, 125, 125),
      GetColor(60, 60, 60)
  );
  AddComponent(std::move(ToggleBtn));

  // Configボタン
  auto ConfigBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto ConfigBtnPtr = ConfigBtn.get();
  ConfigBtnPtr->SetAnchoredPosition({0.0f, 80.0f});
  AddComponent(std::move(ConfigBtn));

  // Exitボタン
  auto ExitBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto ExitBtnPtr = ExitBtn.get();
  ExitBtnPtr->SetAnchoredPosition({0.0f, 240.0f});
  AddComponent(std::move(ExitBtn));

  // --- ナビゲーションの手動リンク設定 ---
  StartBtnPtr->Navigation.Down = ToggleBtnPtr;
  auto startText = std::make_unique<UITextComponent>("Start", GetColor(255, 255, 255), 24);
  startText->SetParentComponent(StartBtnPtr);  // ボタンの子要素にするだけで中央に配置される
  AddComponent(std::move(startText));

  ToggleBtnPtr->Navigation.Up = StartBtnPtr;
  ToggleBtnPtr->Navigation.Down = ConfigBtnPtr;

  auto toggleText = std::make_unique<UITextComponent>("Toggle: OFF", GetColor(255, 255, 255), 24);
  auto ToggleTextPtr = toggleText.get();
  ToggleTextPtr->SetParentComponent(ToggleBtnPtr);
  AddComponent(std::move(toggleText));

  ConfigBtnPtr->Navigation.Up = ToggleBtnPtr;
  ConfigBtnPtr->Navigation.Down = ExitBtnPtr;

  ExitBtnPtr->Navigation.Up = ConfigBtnPtr;

  // ボタン押下時の処理
  StartBtnPtr->OnPressed = []() { M_LOG("Start Game!"); };
  ToggleBtnPtr->OnToggled = [ToggleTextPtr](bool bIsOn) {
    ToggleTextPtr->SetText(bIsOn ? "Toggle: ON" : "Toggle: OFF");
    M_LOG("Toggle Button: {}", bIsOn ? "ON" : "OFF");
  };
  ExitBtnPtr->OnPressed = []() {
    M_LOG("Exit Game!");
    Application::QuitGame();
  };
  // 最初にフォーカスを当てるボタンを設定
  SetFocusedButton(StartBtnPtr);
}
