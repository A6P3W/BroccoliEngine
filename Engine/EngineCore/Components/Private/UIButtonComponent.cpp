#include "UIButtonComponent.h"

void MUIButtonComponent::OnUpdate(float DeltaTime) { MUIWidgetComponent::OnUpdate(DeltaTime); }

void MUIButtonComponent::Press() {
  if (OnPressed) {
    OnPressed();
  }
}

void MUIButtonComponent::SetState(EButtonState NewState) {
  if (NewState == ButtonState) return;
  ButtonState = NewState;
  OnStateChanged(NewState);
}
