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

  auto* VerticalBox = NewObject<MUIVerticalBoxComponent>(this);
  auto* VerticalBoxPtr = VerticalBox;
  VerticalBoxPtr->SetAnchor(EUIAnchor::MiddleCenter);
  VerticalBoxPtr->SetPivot({0.5f, 0.5f});
  VerticalBoxPtr->SetWidgetSize({btnWidth, 0.0f});
  VerticalBoxPtr->SetSpacing(24.0f);
  VerticalBoxPtr->SetAutoResize(true);
  VerticalBox->RegisterComponent();

  // Startボタン
  auto* StartBtn = NewObject<UIBoxButtonComponent>(this);
  auto* StartBtnPtr = StartBtn;
  StartBtnPtr->SetSize(btnWidth, btnHeight);
  StartBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  VerticalBoxPtr->AddItem(StartBtnPtr);
  StartBtn->RegisterComponent();

  // Toggleボタン
  auto* ToggleBtn = NewObject<UIToggleButtonComponent>(this);
  auto* ToggleBtnPtr = ToggleBtn;
  ToggleBtnPtr->SetSize(btnWidth, btnHeight);
  ToggleBtnPtr->SetColors(
      FColor{46, 160, 67},
      FColor{61, 184, 82},
      FColor{35, 134, 54},
      FColor{90, 90, 90},
      FColor{125, 125, 125},
      FColor{60, 60, 60}
  );
  VerticalBoxPtr->AddItem(ToggleBtnPtr);
  ToggleBtn->RegisterComponent();

  // Configボタン
  auto* ConfigBtn = NewObject<UIBoxButtonComponent>(this);
  auto* ConfigBtnPtr = ConfigBtn;
  ConfigBtnPtr->SetSize(btnWidth, btnHeight);
  ConfigBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  VerticalBoxPtr->AddItem(ConfigBtnPtr);
  ConfigBtn->RegisterComponent();

  // 入力可能テキスト欄
  auto* NameInput = NewObject<UIInputTextComponent>(this);
  auto* NameInputPtr = NameInput;
  NameInputPtr->SetSize(btnWidth, btnHeight);
  NameInputPtr->SetHintText("Name...");
  NameInputPtr->SetMaxLength(12);
  NameInputPtr->SetColors(normalColor, hoveredColor, inputEditingColor);
  VerticalBoxPtr->AddItem(NameInputPtr);
  NameInput->RegisterComponent();

  // Exitボタン
  auto* ExitBtn = NewObject<UIBoxButtonComponent>(this);
  auto* ExitBtnPtr = ExitBtn;
  ExitBtnPtr->SetSize(btnWidth, btnHeight);
  ExitBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  VerticalBoxPtr->AddItem(ExitBtnPtr);
  ExitBtn->RegisterComponent();

  auto* LeftNavBtn = NewObject<UIBoxButtonComponent>(this);
  auto* LeftNavBtnPtr = LeftNavBtn;
  LeftNavBtnPtr->SetSize(sideBtnWidth, sideBtnHeight);
  LeftNavBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
  LeftNavBtnPtr->SetAnchoredPosition({-260.0f, 0.0f});
  LeftNavBtn->RegisterComponent();

  auto* RightNavBtn = NewObject<UIBoxButtonComponent>(this);
  auto* RightNavBtnPtr = RightNavBtn;
  RightNavBtnPtr->SetSize(sideBtnWidth, sideBtnHeight);
  RightNavBtnPtr->SetColors(normalColor, hoveredColor, pressedColor);
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

  // ボタン押下時の処理
  StartBtnPtr->SetOnPressed([]() { M_LOG("Start Game!"); });
  ToggleBtnPtr->SetOnToggled([ToggleTextPtr](bool bIsOn) {
    ToggleTextPtr->SetText(bIsOn ? "Toggle: ON" : "Toggle: OFF");
    M_LOG("Toggle Button: {}", bIsOn ? "ON" : "OFF");
  });
  NameInputPtr->SetOnTextChanged([](const std::string& text) { M_LOG("Input changed: {}", text); });
  NameInputPtr->SetOnTextCommitted([CommittedTextPtr](const std::string& text) {
    const std::string displayText = text.empty() ? "<empty>" : text;
    CommittedTextPtr->SetText("Input: " + displayText);
    M_LOG("Input committed: {}", displayText);
  });
  ExitBtnPtr->SetOnPressed([]() {
    M_LOG("Exit Game!");
    Application::QuitGame();
  });
  LeftNavBtnPtr->SetOnPressed([]() { M_LOG("Left navigation button pressed."); });
  RightNavBtnPtr->SetOnPressed([]() { M_LOG("Right navigation button pressed."); });

  VerticalBoxPtr->BuildNavigation();
  VerticalBoxPtr->SetNavigationLeft(LeftNavBtnPtr);
  VerticalBoxPtr->SetNavigationRight(RightNavBtnPtr);
  LeftNavBtnPtr->Navigation.Right = StartBtnPtr;
  RightNavBtnPtr->Navigation.Left = StartBtnPtr;

  // 最初にフォーカスを当てるボタンを設定
  SetFocusedButton(StartBtnPtr);
}
