#include "UIBoxButton.h"

#include <memory>

#include "Actor.h"
#include "SpriteComponent.h"

UIBoxButtonComponent::UIBoxButtonComponent(
    float width, float height, int normalColor, int hoveredColor, int pressedColor
)
    : Width(width),
      Height(height),
      NormalColor(normalColor),
      HoveredColor(hoveredColor),
      PressedColor(pressedColor) {
  SetWidgetSize({width, height});
}

void UIBoxButtonComponent::RegisterComponent() {
  if (BoxSprite != nullptr || GetOwner() == nullptr) {
    return;
  }
  auto boxSprite = std::make_unique<MSpriteComponent>(GetFinalPriority(), RenderSpace::Screen);
  BoxSprite = boxSprite.get();
  BoxSprite->SetParentComponent(this);
  UpdateBox();
  GetOwner()->AddComponent(std::move(boxSprite));
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
