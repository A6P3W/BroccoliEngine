#pragma once
#include "BroccoliEngineAPI.h"
#include "UIButtonComponent.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API UIBoxButtonComponent : public MUIButtonComponent {
 public:
  UIBoxButtonComponent();

  void OnRegister() override;
  void OnStateChanged(EButtonState NewState) override;

  void SetSize(float width, float height);
  void SetColors(int normalColor, int hoveredColor, int pressedColor);

 private:
  void UpdateBox();
  int GetColorForState(EButtonState State) const;

  MSpriteComponent* BoxSprite = nullptr;
  float Width = 0.0f;
  float Height = 0.0f;
  int NormalColor = 0;
  int HoveredColor = 0;
  int PressedColor = 0;
  EButtonState CurrentState = EButtonState::Normal;
};
