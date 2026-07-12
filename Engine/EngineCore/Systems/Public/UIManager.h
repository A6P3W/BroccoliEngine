#pragma once
#include "BroccoliEngineAPI.h"
#include <vector>

#include "UMath.h"

struct FInputActionValue;
class AWidgetBase;

class BROCCOLI_ENGINE_API UIManager {
 public:
  static UIManager* GetInstance();

  void AddWidget(AWidgetBase* Widget);
  void RemoveWidget(AWidgetBase* Widget);

  void Navigate(const FInputActionValue& Value);
  void Submit();
  void Cancel();

  void SetTextInputActive(bool bActive);
  bool IsTextInputActive() const;

  void SetFocusedWidget(AWidgetBase* Widget);
  AWidgetBase* GetFocusedWidget() const;

 private:
  UIManager();
  ~UIManager();
  void RefreshZOrder();

  struct Impl;
  Impl* ImplPtr = nullptr;
};
