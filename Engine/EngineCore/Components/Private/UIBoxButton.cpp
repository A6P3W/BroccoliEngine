#include "UIBoxButton.h"

#include <memory>

#include "Actor.h"
#include "SpriteComponent.h"

UIBoxButtonComponent::UIBoxButtonComponent() {
  SetWidgetSize({Width, Height});
}

void UIBoxButtonComponent::OnRegister() {
  if (BoxSprite != nullptr || GetOwner() == nullptr) {
    return;
  }
  BoxSprite = NewObject<MSpriteComponent>(GetOwner());
  if (BoxSprite == nullptr) {
    return;
  }
  BoxSprite->SetRenderSettings(GetFinalPriority(), RenderSpace::Screen);
  BoxSprite->AttachToComponent(this);
  UpdateBox();
  BoxSprite->RegisterComponent();
}

void UIBoxButtonComponent::OnStateChanged(EButtonState NewState) {
  CurrentState = NewState;
  UpdateBox();
}

void UIBoxButtonComponent::SetSize(float width, float height) {
  Width = width;
  Height = height;
  SetWidgetSize({width, height});
  UpdateBox();
}

void UIBoxButtonComponent::SetColors(int normalColor, int hoveredColor, int pressedColor) {
  NormalColor = normalColor;
  HoveredColor = hoveredColor;
  PressedColor = pressedColor;
  UpdateBox();
}

void UIBoxButtonComponent::UpdateBox() {
  if (BoxSprite == nullptr) {
    return;
  }
  BoxSprite->SetRelativeLocation({-Width * 0.5f, -Height * 0.5f});
  BoxSprite->SubmitBox(Width, Height, GetColorForState(CurrentState), true);
}

int UIBoxButtonComponent::GetColorForState(EButtonState State) const {
  switch (State) {
    case EButtonState::Hovered:
      return HoveredColor;
    case EButtonState::Pressed:
      return PressedColor;
    case EButtonState::Normal:
    case EButtonState::Disabled:
    default:
      return NormalColor;
  }
}
