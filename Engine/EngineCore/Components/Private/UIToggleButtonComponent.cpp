#include "UIToggleButtonComponent.h"

#include <memory>

#include "Actor.h"
#include "SpriteComponent.h"

struct UIToggleButtonComponent::Impl {
  MSpriteComponent* BoxSprite = nullptr;
  bool bIsOn = false;
  float Width = 0.0f;
  float Height = 0.0f;

  int OnNormalColor = 0;
  int OnHoveredColor = 0;
  int OnPressedColor = 0;

  int OffNormalColor = 0;
  int OffHoveredColor = 0;
  int OffPressedColor = 0;

  EButtonState CurrentState = EButtonState::Normal;
  std::function<void(bool)> OnToggled;
};

UIToggleButtonComponent::UIToggleButtonComponent() : ImplPtr(new Impl()) {
  SetWidgetSize({ImplPtr->Width, ImplPtr->Height});
}

UIToggleButtonComponent::~UIToggleButtonComponent() { delete ImplPtr; }

void UIToggleButtonComponent::OnRegister() {
  if (ImplPtr->BoxSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  ImplPtr->BoxSprite = NewObject<MSpriteComponent>(GetOwner());
  if (ImplPtr->BoxSprite == nullptr) {
    return;
  }
  ImplPtr->BoxSprite->SetRenderSettings(GetFinalPriority(), RenderSpace::Screen);
  ImplPtr->BoxSprite->AttachToComponent(this);
  UpdateVisuals();
  ImplPtr->BoxSprite->RegisterComponent();
}

void UIToggleButtonComponent::Press() {
  if (ImplPtr->CurrentState == EButtonState::Disabled) {
    return;
  }

  Toggle();
  MUIButtonComponent::Press();
}

void UIToggleButtonComponent::OnStateChanged(EButtonState NewState) {
  ImplPtr->CurrentState = NewState;
  UpdateVisuals();
}

void UIToggleButtonComponent::SetIsOn(bool bNewIsOn, bool bBroadcast) {
  if (ImplPtr->bIsOn == bNewIsOn) {
    return;
  }

  ImplPtr->bIsOn = bNewIsOn;
  UpdateVisuals();

  if (bBroadcast && ImplPtr->OnToggled) {
    ImplPtr->OnToggled(ImplPtr->bIsOn);
  }
}

bool UIToggleButtonComponent::GetIsOn() const { return ImplPtr->bIsOn; }

void UIToggleButtonComponent::SetOnToggled(std::function<void(bool)> Callback) {
  ImplPtr->OnToggled = std::move(Callback);
}

void UIToggleButtonComponent::SetSize(float width, float height) {
  ImplPtr->Width = width;
  ImplPtr->Height = height;
  SetWidgetSize({width, height});
  UpdateVisuals();
}

void UIToggleButtonComponent::SetColors(
    int onNormal, int onHovered, int onPressed, int offNormal, int offHovered, int offPressed
) {
  ImplPtr->OnNormalColor = onNormal;
  ImplPtr->OnHoveredColor = onHovered;
  ImplPtr->OnPressedColor = onPressed;
  ImplPtr->OffNormalColor = offNormal;
  ImplPtr->OffHoveredColor = offHovered;
  ImplPtr->OffPressedColor = offPressed;
  UpdateVisuals();
}

void UIToggleButtonComponent::Toggle() {
  if (ImplPtr->CurrentState == EButtonState::Disabled) {
    return;
  }

  SetIsOn(!ImplPtr->bIsOn, true);
}

void UIToggleButtonComponent::UpdateVisuals() {
  if (ImplPtr->BoxSprite == nullptr) {
    return;
  }

  ImplPtr->BoxSprite->SetRelativeLocation({-ImplPtr->Width * 0.5f, -ImplPtr->Height * 0.5f});
  ImplPtr->BoxSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, GetCurrentColor(), true);
}

int UIToggleButtonComponent::GetCurrentColor() const {
  if (ImplPtr->bIsOn) {
    switch (ImplPtr->CurrentState) {
      case EButtonState::Hovered:
        return ImplPtr->OnHoveredColor;
      case EButtonState::Pressed:
        return ImplPtr->OnPressedColor;
      case EButtonState::Normal:
      case EButtonState::Disabled:
      default:
        return ImplPtr->OnNormalColor;
    }
  }

  switch (ImplPtr->CurrentState) {
    case EButtonState::Hovered:
      return ImplPtr->OffHoveredColor;
    case EButtonState::Pressed:
      return ImplPtr->OffPressedColor;
    case EButtonState::Normal:
    case EButtonState::Disabled:
    default:
      return ImplPtr->OffNormalColor;
  }
}
