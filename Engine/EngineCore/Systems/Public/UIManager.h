#pragma once
#include <vector>

#include "UMath.h"

struct FInputActionValue;
class AWidgetBase;

class UIManager {
 public:
  static UIManager* GetInstance() {
    static UIManager instance;
    return &instance;
  }

  void AddWidget(AWidgetBase* Widget);
  void RemoveWidget(AWidgetBase* Widget);

  void Navigate(const FInputActionValue& Value);
  void Submit();
  void Cancel();

  void SetTextInputActive(bool bActive) { bTextInputActive = bActive; }
  bool IsTextInputActive() const { return bTextInputActive; }

  void SetFocusedWidget(AWidgetBase* Widget) { CurrentFocusedWidget = Widget; }
  AWidgetBase* GetFocusedWidget() const { return CurrentFocusedWidget; }

 private:
  void RefreshZOrder();

  AWidgetBase* CurrentFocusedWidget = nullptr;
  std::vector<AWidgetBase*> ActiveWidgets;
  bool bTextInputActive = false;
};
