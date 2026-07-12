#pragma once
#include "BroccoliEngineAPI.h"
#include <functional>

#include "UIButtonComponent.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API UIToggleButtonComponent : public MUIButtonComponent {
 public:
  UIToggleButtonComponent();

  void OnRegister() override;
  void Press() override;
  void OnStateChanged(EButtonState NewState) override;

  void SetIsOn(bool bNewIsOn, bool bBroadcast = true);
  bool GetIsOn() const { return bIsOn; }

  std::function<void(bool)> OnToggled;

  void SetSize(float width, float height);
  void SetColors(
      int onNormal, int onHovered, int onPressed, int offNormal, int offHovered, int offPressed
  );

 private:
  void Toggle();
  void UpdateVisuals();
  int GetCurrentColor() const;

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
};
