#pragma once
#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UIButtonComponent.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API UIBoxButtonComponent : public MUIButtonComponent {
 public:
  UIBoxButtonComponent();

  void OnRegister() override;
  void OnStateChanged(EButtonState NewState) override;

  void SetSize(float width, float height);
  void SetColors(const FColor& normalColor, const FColor& hoveredColor, const FColor& pressedColor);

 private:
  void UpdateBox();
  FColor GetColorForState(EButtonState State) const;

  MSpriteComponent* BoxSprite = nullptr;
  float Width = 0.0f;
  float Height = 0.0f;
  FColor NormalColor = FColor::Black;
  FColor HoveredColor = FColor::Black;
  FColor PressedColor = FColor::Black;
  EButtonState CurrentState = EButtonState::Normal;
};
