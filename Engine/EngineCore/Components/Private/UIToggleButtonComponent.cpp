#include "UIToggleButtonComponent.h"

#include <memory>

#include "Actor.h"
#include "SpriteComponent.h"

UIToggleButtonComponent::UIToggleButtonComponent() {
  SetWidgetSize({Width, Height});
}

void UIToggleButtonComponent::OnRegister() {
  if (BoxSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  BoxSprite = NewObject<MSpriteComponent>(GetOwner());
  if (BoxSprite == nullptr) {
    return;
  }
  BoxSprite->SetRenderSettings(GetFinalPriority(), RenderSpace::Screen);
  BoxSprite->AttachToComponent(this);
  UpdateVisuals();
  BoxSprite->RegisterComponent();
}

void UIToggleButtonComponent::Press() {
  if (CurrentState == EButtonState::Disabled) {
    return;
  }

  Toggle();
  MUIButtonComponent::Press();
}

void UIToggleButtonComponent::OnStateChanged(EButtonState NewState) {
  CurrentState = NewState;
  UpdateVisuals();
}

void UIToggleButtonComponent::SetIsOn(bool bNewIsOn, bool bBroadcast) {
  if (bIsOn == bNewIsOn) {
    return;
  }

  bIsOn = bNewIsOn;
  UpdateVisuals();

  if (bBroadcast && OnToggled) {
    OnToggled(bIsOn);
  }
}

void UIToggleButtonComponent::SetSize(float width, float height) {
  Width = width;
  Height = height;
  SetWidgetSize({width, height});
  UpdateVisuals();
}

void UIToggleButtonComponent::SetColors(
    int onNormal, int onHovered, int onPressed, int offNormal, int offHovered, int offPressed
) {
  OnNormalColor = onNormal;
  OnHoveredColor = onHovered;
  OnPressedColor = onPressed;
  OffNormalColor = offNormal;
  OffHoveredColor = offHovered;
  OffPressedColor = offPressed;
  UpdateVisuals();
}

void UIToggleButtonComponent::Toggle() {
  if (CurrentState == EButtonState::Disabled) {
    return;
  }

  SetIsOn(!bIsOn, true);
}

void UIToggleButtonComponent::UpdateVisuals() {
  if (BoxSprite == nullptr) {
    return;
  }

  BoxSprite->SetRelativeLocation({-Width * 0.5f, -Height * 0.5f});
  BoxSprite->SubmitBox(Width, Height, GetCurrentColor(), true);
}

int UIToggleButtonComponent::GetCurrentColor() const {
  if (bIsOn) {
    switch (CurrentState) {
      case EButtonState::Hovered:
        return OnHoveredColor;
      case EButtonState::Pressed:
        return OnPressedColor;
      case EButtonState::Normal:
      case EButtonState::Disabled:
      default:
        return OnNormalColor;
    }
  }

  switch (CurrentState) {
    case EButtonState::Hovered:
      return OffHoveredColor;
    case EButtonState::Pressed:
      return OffPressedColor;
    case EButtonState::Normal:
    case EButtonState::Disabled:
    default:
      return OffNormalColor;
  }
}
