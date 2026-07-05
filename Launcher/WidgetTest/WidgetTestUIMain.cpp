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
  int normalColor = GetColor(100, 100, 100);
  int hoveredColor = GetColor(150, 150, 150);
  int pressedColor = GetColor(50, 50, 50);
  int inputEditingColor = GetColor(40, 100, 170);

  auto VerticalBox = std::make_unique<MUIVerticalBoxComponent>();
  auto VerticalBoxPtr = VerticalBox.get();
  VerticalBoxPtr->SetAnchor(EUIAnchor::MiddleCenter);
  VerticalBoxPtr->SetPivot({0.5f, 0.5f});
  VerticalBoxPtr->SetWidgetSize({btnWidth, 0.0f});
  VerticalBoxPtr->SetSpacing(24.0f);
  VerticalBoxPtr->SetAutoResize(true);
  AddComponent(std::move(VerticalBox));

  // Startボタン
  auto StartBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto StartBtnPtr = StartBtn.get();
  VerticalBoxPtr->AddItem(StartBtnPtr);
  AddComponent(std::move(StartBtn));

  // Toggleボタン
  auto ToggleBtn = std::make_unique<UIToggleButtonComponent>(
      btnWidth, btnHeight, GetColor(46, 160, 67), GetColor(90, 90, 90)
  );
  auto ToggleBtnPtr = ToggleBtn.get();
  ToggleBtnPtr->SetColors(
      GetColor(46, 160, 67),
      GetColor(61, 184, 82),
      GetColor(35, 134, 54),
      GetColor(90, 90, 90),
      GetColor(125, 125, 125),
      GetColor(60, 60, 60)
  );
  VerticalBoxPtr->AddItem(ToggleBtnPtr);
  AddComponent(std::move(ToggleBtn));

  // Configボタン
  auto ConfigBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto ConfigBtnPtr = ConfigBtn.get();
  VerticalBoxPtr->AddItem(ConfigBtnPtr);
  AddComponent(std::move(ConfigBtn));

  // 入力可能テキスト欄
  auto NameInput = std::make_unique<UIInputTextComponent>(btnWidth, btnHeight, "Name...");
  auto NameInputPtr = NameInput.get();
  NameInputPtr->SetMaxLength(12);
  NameInputPtr->SetColors(normalColor, hoveredColor, inputEditingColor);
  VerticalBoxPtr->AddItem(NameInputPtr);
  AddComponent(std::move(NameInput));

  // Exitボタン
  auto ExitBtn = std::make_unique<UIBoxButtonComponent>(
      btnWidth, btnHeight, normalColor, hoveredColor, pressedColor
  );
  auto ExitBtnPtr = ExitBtn.get();
  VerticalBoxPtr->AddItem(ExitBtnPtr);
  AddComponent(std::move(ExitBtn));

  auto LeftNavBtn = std::make_unique<UIBoxButtonComponent>(
      sideBtnWidth, sideBtnHeight, normalColor, hoveredColor, pressedColor
  );
  auto LeftNavBtnPtr = LeftNavBtn.get();
  LeftNavBtnPtr->SetAnchoredPosition({-260.0f, 0.0f});
  AddComponent(std::move(LeftNavBtn));

  auto RightNavBtn = std::make_unique<UIBoxButtonComponent>(
      sideBtnWidth, sideBtnHeight, normalColor, hoveredColor, pressedColor
  );
  auto RightNavBtnPtr = RightNavBtn.get();
  RightNavBtnPtr->SetAnchoredPosition({260.0f, 0.0f});
  AddComponent(std::move(RightNavBtn));

  auto startText = std::make_unique<UITextComponent>("Start", GetColor(255, 255, 255), 24);
  startText->SetParentComponent(StartBtnPtr);  // ボタンの子要素にするだけで中央に配置される
  AddComponent(std::move(startText));

  auto toggleText = std::make_unique<UITextComponent>("Toggle: OFF", GetColor(255, 255, 255), 24);
  auto ToggleTextPtr = toggleText.get();
  ToggleTextPtr->SetParentComponent(ToggleBtnPtr);
  AddComponent(std::move(toggleText));

  auto leftNavText = std::make_unique<UITextComponent>("Left", GetColor(255, 255, 255), 24);
  leftNavText->SetParentComponent(LeftNavBtnPtr);
  AddComponent(std::move(leftNavText));

  auto rightNavText = std::make_unique<UITextComponent>("Right", GetColor(255, 255, 255), 24);
  rightNavText->SetParentComponent(RightNavBtnPtr);
  AddComponent(std::move(rightNavText));

  auto committedText =
      std::make_unique<UITextComponent>("Input: <empty>", GetColor(210, 230, 255), 20);
  auto CommittedTextPtr = committedText.get();
  CommittedTextPtr->SetAnchor(EUIAnchor::MiddleCenter);
  CommittedTextPtr->SetAnchoredPosition({0.0f, 210.0f});
  AddComponent(std::move(committedText));

  // ボタン押下時の処理
  StartBtnPtr->OnPressed = []() { M_LOG("Start Game!"); };
  ToggleBtnPtr->OnToggled = [ToggleTextPtr](bool bIsOn) {
    ToggleTextPtr->SetText(bIsOn ? "Toggle: ON" : "Toggle: OFF");
    M_LOG("Toggle Button: {}", bIsOn ? "ON" : "OFF");
  };
  NameInputPtr->OnTextChanged = [](const std::string& text) {
    M_LOG("Input changed: {}", text);
  };
  NameInputPtr->OnTextCommitted = [CommittedTextPtr](const std::string& text) {
    const std::string displayText = text.empty() ? "<empty>" : text;
    CommittedTextPtr->SetText("Input: " + displayText);
    M_LOG("Input committed: {}", displayText);
  };
  ExitBtnPtr->OnPressed = []() {
    M_LOG("Exit Game!");
    Application::QuitGame();
  };
  LeftNavBtnPtr->OnPressed = []() { M_LOG("Left navigation button pressed."); };
  RightNavBtnPtr->OnPressed = []() { M_LOG("Right navigation button pressed."); };

  VerticalBoxPtr->BuildNavigation();
  VerticalBoxPtr->SetNavigationLeft(LeftNavBtnPtr);
  VerticalBoxPtr->SetNavigationRight(RightNavBtnPtr);
  LeftNavBtnPtr->Navigation.Right = StartBtnPtr;
  RightNavBtnPtr->Navigation.Left = StartBtnPtr;

  // 最初にフォーカスを当てるボタンを設定
  SetFocusedButton(StartBtnPtr);
}
