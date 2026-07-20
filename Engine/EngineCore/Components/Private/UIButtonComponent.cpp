#include "UIButtonComponent.h"

struct MUIButtonComponent::Impl {
  std::function<void()> OnPressed;
  EButtonState ButtonState = EButtonState::Normal;
  bool bAllowGamepadSubmit = true;
};

MUIButtonComponent::MUIButtonComponent() : ImplPtr(new Impl()) {}

MUIButtonComponent::~MUIButtonComponent() { delete ImplPtr; }

void MUIButtonComponent::OnUpdate(float DeltaTime) { MUIWidgetComponent::OnUpdate(DeltaTime); }

void MUIButtonComponent::SetOnPressed(std::function<void()> Callback) {
  ImplPtr->OnPressed = std::move(Callback);
}

void MUIButtonComponent::Press() {
  if (ImplPtr->OnPressed) {
    ImplPtr->OnPressed();
  }
}

void MUIButtonComponent::SetAllowGamepadSubmit(bool Allow) { ImplPtr->bAllowGamepadSubmit = Allow; }

bool MUIButtonComponent::IsGamepadSubmitAllowed() const { return ImplPtr->bAllowGamepadSubmit; }

void MUIButtonComponent::SetState(EButtonState NewState) {
  if (NewState == ImplPtr->ButtonState) return;
  ImplPtr->ButtonState = NewState;
  OnStateChanged(NewState);
}
