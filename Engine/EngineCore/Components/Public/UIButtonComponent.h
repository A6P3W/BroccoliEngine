#pragma once
#include <functional>

#include "UIWidgetComponent.h"
enum class EButtonState { Normal, Hovered, Pressed, Disabled };

class MUIButtonComponent : public MUIWidgetComponent {
 public:
  struct FNavigationLinks {
    MUIButtonComponent* Up = nullptr;
    MUIButtonComponent* Down = nullptr;
    MUIButtonComponent* Left = nullptr;
    MUIButtonComponent* Right = nullptr;
  };

  void OnUpdate(float DeltaTime) override;
  std::function<void()> OnPressed;

  virtual void Press();
  virtual void OnStateChanged(EButtonState NewState) {}
  void SetState(EButtonState NewState);

  FNavigationLinks Navigation;

 private:
  EButtonState ButtonState = EButtonState::Normal;
};
