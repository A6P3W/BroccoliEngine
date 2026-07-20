#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"

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
  void ClearFocusedButton();
  MUIButtonComponent* GetFocusedButton() const { return FocusedButtonComponent; }
  virtual void Navigate(const FInputActionValue& Value);
  void Submit(const FInputActionValue& Value);
  virtual void Cancel();

  void SetZOrderOffset(int offset);

 protected:
  void StartNavigationCooldown();

 private:
  MUIButtonComponent* FocusedButtonComponent = nullptr;
  int ZOrderOffset = 0;
  float NavigationCooldown = 0.0f;
};
