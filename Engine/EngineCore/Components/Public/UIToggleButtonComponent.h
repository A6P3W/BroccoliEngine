#pragma once
#include <functional>

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UIButtonComponent.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API UIToggleButtonComponent : public MUIButtonComponent {
 public:
  UIToggleButtonComponent();
  ~UIToggleButtonComponent() override;

  void OnRegister() override;
  void Press() override;
  void OnStateChanged(EButtonState NewState) override;

  void SetIsOn(bool bNewIsOn, bool bBroadcast = true);
  bool GetIsOn() const;

  void SetOnToggled(std::function<void(bool)> Callback);

  void SetSize(float width, float height);
  void SetColors(
      const FColor& onNormal,
      const FColor& onHovered,
      const FColor& onPressed,
      const FColor& offNormal,
      const FColor& offHovered,
      const FColor& offPressed
  );

 private:
  void Toggle();
  void UpdateVisuals();
  FColor GetCurrentColor() const;

  struct Impl;
  Impl* ImplPtr = nullptr;
};
