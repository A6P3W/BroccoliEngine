#include "LevelStarterWidget.h"

#include <DxLib.h>

#include <memory>
#include <string>

#include "FileDialog.h"
#include "SceneManager.h"
#include "UIBoxButton.h"
#include "UIManager.h"
#include "UITextComponent.h"

REGISTER_ACTOR(ALevelStarterWidget)

ALevelStarterWidget::ALevelStarterWidget() {
  constexpr float ButtonWidth = 300.0f;
  constexpr float ButtonHeight = 60.0f;

  auto* buttonPtr = NewObject<UIBoxButtonComponent>(this);
  buttonPtr->SetSize(ButtonWidth, ButtonHeight);
  buttonPtr->SetColors(GetColor(85, 85, 85), GetColor(119, 119, 119), GetColor(51, 51, 51));
  buttonPtr->SetAnchor(EUIAnchor::MiddleCenter);
  buttonPtr->OnPressed = []() {
    const std::string filepath =
        FileDialog::OpenFile("Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0");
    if (!filepath.empty()) {
      SceneManager::GetInstance().OpenLevelByPath(filepath);
    }
  };

  auto* text = NewObject<UITextComponent>(this);
  text->SetText("Select .BLevel to Play");
  text->SetColor(GetColor(255, 255, 255));
  text->SetFontSize(24);
  text->AttachToComponent(buttonPtr);
  text->SetAnchor(EUIAnchor::MiddleCenter);

  OpenButton = buttonPtr;

  buttonPtr->RegisterComponent();
  text->RegisterComponent();
}

void ALevelStarterWidget::BeginPlay() {
  AWidgetBase::BeginPlay();

  if (auto* uiManager = UIManager::GetInstance()) {
    uiManager->AddWidget(this);
    uiManager->SetFocusedWidget(this);
  }

  SetFocusedButton(OpenButton);
}
