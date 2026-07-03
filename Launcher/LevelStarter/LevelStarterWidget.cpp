#include "LevelStarterWidget.h"

#include <DxLib.h>

#include <memory>
#include <string>

#include "EngineDefine.h"
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

void ALevelStarterWidget::OnUpdate(float DeltaTime) {
  AWidgetBase::OnUpdate(DeltaTime);

  if (!OpenButton) {
    return;
  }

  int mouseX = 0;
  int mouseY = 0;
  GetMousePoint(&mouseX, &mouseY);

  int screenW = 0;
  int screenH = 0;
  GetDrawScreenSize(&screenW, &screenH);
  if (screenW <= 0 || screenH <= 0) {
    return;
  }

  const float scaleX = static_cast<float>(screenW) / static_cast<float>(VirtualWidth);
  const float scaleY = static_cast<float>(screenH) / static_cast<float>(VirtualHeight);
  const float scale = (scaleX < scaleY) ? scaleX : scaleY;
  if (scale <= 0.0f) {
    return;
  }

  const float drawW = static_cast<float>(VirtualWidth) * scale;
  const float drawH = static_cast<float>(VirtualHeight) * scale;
  const float drawX = (static_cast<float>(screenW) - drawW) * 0.5f;
  const float drawY = (static_cast<float>(screenH) - drawH) * 0.5f;
  const FVector2D virtualMouse = {
      (static_cast<float>(mouseX) - drawX) / scale,
      (static_cast<float>(mouseY) - drawY) / scale
  };

  const FVector2D buttonLocation = OpenButton->GetWorldLocation();
  const FVector2D buttonSize = OpenButton->GetWidgetSize();
  const bool bHovered =
      virtualMouse.X >= buttonLocation.X - buttonSize.X * 0.5f &&
      virtualMouse.X <= buttonLocation.X + buttonSize.X * 0.5f &&
      virtualMouse.Y >= buttonLocation.Y - buttonSize.Y * 0.5f &&
      virtualMouse.Y <= buttonLocation.Y + buttonSize.Y * 0.5f;

  const bool bMouseLeftDown = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
  if (bHovered) {
    SetFocusedButton(OpenButton);
    if (bMouseLeftDown && !bWasMouseLeftDown && OpenButton->OnPressed) {
      OpenButton->OnPressed();
    }
  }

  bWasMouseLeftDown = bMouseLeftDown;
}