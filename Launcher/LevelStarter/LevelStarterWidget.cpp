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

  auto button = std::make_unique<UIBoxButtonComponent>(
      ButtonWidth,
      ButtonHeight,
      GetColor(85, 85, 85),
      GetColor(119, 119, 119),
      GetColor(51, 51, 51)
  );
  UIBoxButtonComponent* buttonPtr = button.get();
  buttonPtr->SetAnchor(EUIAnchor::MiddleCenter);
  buttonPtr->OnPressed = []() {
    const std::string filepath = FileDialog::OpenFile(
        "Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0"
    );
    if (!filepath.empty()) {
      SceneManager::GetInstance().OpenLevelByPath(filepath);
    }
  };

  auto text =
      std::make_unique<UITextComponent>("Select .BLevel to Play", GetColor(255, 255, 255), 24);
  text->SetParentComponent(buttonPtr);
  text->SetAnchor(EUIAnchor::MiddleCenter);

  OpenButton = buttonPtr;

  AddComponent(std::move(button));
  AddComponent(std::move(text));
}

void ALevelStarterWidget::BeginPlay() {
  AWidgetBase::BeginPlay();

  if (auto* uiManager = UIManager::GetInstance()) {
    uiManager->AddWidget(this);
    uiManager->SetFocusedWidget(this);
  }

  SetFocusedButton(OpenButton);
}
