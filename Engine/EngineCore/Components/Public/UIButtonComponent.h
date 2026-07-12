#pragma once
#include "BroccoliEngineAPI.h"
#include <functional>

#include "UIWidgetComponent.h"
enum class EButtonState { Normal, Hovered, Pressed, Disabled };

class BROCCOLI_ENGINE_API MUIButtonComponent : public MUIWidgetComponent {
 public:
  struct FNavigationLinks {
    MUIButtonComponent* Up = nullptr;
    MUIButtonComponent* Down = nullptr;
    MUIButtonComponent* Left = nullptr;
    MUIButtonComponent* Right = nullptr;
  };

  MUIButtonComponent();
  ~MUIButtonComponent() override;

  void OnUpdate(float DeltaTime) override;
  void SetOnPressed(std::function<void()> Callback);

  virtual void Press();
  virtual void OnStateChanged(EButtonState NewState) {}
  void SetState(EButtonState NewState);

  FNavigationLinks Navigation;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};