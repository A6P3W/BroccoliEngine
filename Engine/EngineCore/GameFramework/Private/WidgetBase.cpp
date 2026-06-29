#include "WidgetBase.h"

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "UIButtonComponent.h"
#include "UIManager.h"
AWidgetBase::~AWidgetBase() {
  if (auto* uiManager = UIManager::GetInstance()) {
    if (uiManager->GetFocusedWidget() == this) {
      uiManager->SetFocusedWidget(nullptr);
    }
    uiManager->RemoveWidget(this);
  }
}
void AWidgetBase::OnUpdate(float DeltaTime) {
  if (NavigationCooldown > 0.0f) {
    NavigationCooldown -= DeltaTime;
  }
}
void AWidgetBase::SetFocusedButton(MUIButtonComponent* Button) {
  if (FocusedButtonComponent) {
    FocusedButtonComponent->SetState(EButtonState::Normal);
  }
  FocusedButtonComponent = Button;
  if (FocusedButtonComponent) {
    FocusedButtonComponent->SetState(EButtonState::Hovered);
  }
}
void AWidgetBase::Navigate(const FInputActionValue& Value) {
  if (!FocusedButtonComponent || NavigationCooldown > 0.0f) return;
  const float Threshold = 0.5f;
  MUIButtonComponent* NextButton = nullptr;

  if (Value.Axis2D.Y > Threshold) {
    NextButton = FocusedButtonComponent->Navigation.Up;
  } else if (Value.Axis2D.Y < -Threshold) {
    NextButton = FocusedButtonComponent->Navigation.Down;
  } else if (Value.Axis2D.X < -Threshold) {
    NextButton = FocusedButtonComponent->Navigation.Right;
  } else if (Value.Axis2D.X > Threshold) {
    NextButton = FocusedButtonComponent->Navigation.Left;
  }

  if (NextButton) {
    SetFocusedButton(NextButton);
    NavigationCooldown = 0.2f;
  }
}

void AWidgetBase::Submit() {
  if (FocusedButtonComponent) {
    FocusedButtonComponent->OnPressed();
  }
}

void AWidgetBase::Cancel() {}

void AWidgetBase::SetZOrderOffset(int offset) {
  ZOrderOffset = offset;

  auto widgets = GetComponents<MUIWidgetComponent>();
  for (auto* widget : widgets) {
    widget->SetZOrderOffset(offset);
  }
}
