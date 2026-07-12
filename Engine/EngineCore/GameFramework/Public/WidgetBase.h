#pragma once
#include "BroccoliEngineAPI.h"
#include "Actor.h"

struct FInputActionValue;
class MUIButtonComponent;

class BROCCOLI_ENGINE_API AWidgetBase : public AActor {
 public:
  virtual ~AWidgetBase() override;

  void OnUpdate(float DeltaTime) override;

  virtual void OnOpened() {}
  virtual void OnClosed() {}
  virtual void OnObscured() {}
  virtual void OnRevealed() {}

  void SetFocusedButton(MUIButtonComponent* Button);
  MUIButtonComponent* GetFocusedButton() const { return FocusedButtonComponent; }
  void Navigate(const FInputActionValue& Value);
  void Submit();
  virtual void Cancel();

  void SetZOrderOffset(int offset);

 private:
  MUIButtonComponent* FocusedButtonComponent = nullptr;
  int ZOrderOffset = 0;
  float NavigationCooldown = 0.0f;
};
