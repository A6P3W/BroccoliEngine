#include "UIManager.h"

#include <algorithm>

#include "Actor.h"
#include "EnhancedInputComponent.h"
#include "Log.h"
#include "WidgetBase.h"

void UIManager::RefreshZOrder() {
  int currentOffset = 0;
  for (AWidgetBase* widget : ActiveWidgets) {
    if (widget) {
      widget->SetZOrderOffset(currentOffset);
      currentOffset += 100;
    }
  }
}

void UIManager::AddWidget(AWidgetBase* Widget) {
  if (!Widget) return;

  ActiveWidgets.push_back(Widget);
  Widget->OnOpened();

  RefreshZOrder();
}

void UIManager::RemoveWidget(AWidgetBase* Widget) {
  if (!Widget) return;

  if (CurrentFocusedWidget == Widget) {
    CurrentFocusedWidget = nullptr;
    bTextInputActive = false;
  }

  auto it = std::find(ActiveWidgets.begin(), ActiveWidgets.end(), Widget);
  if (it != ActiveWidgets.end()) {
    (*it)->OnClosed();
    (*it)->Destroy();
    ActiveWidgets.erase(it);

    RefreshZOrder();
  }
}

void UIManager::Navigate(const FInputActionValue& Value) {
  if (bTextInputActive) {
    return;
  }

  if (CurrentFocusedWidget) {
    CurrentFocusedWidget->Navigate(Value);
  }
}

void UIManager::Submit() {
  if (bTextInputActive) {
    return;
  }

  if (CurrentFocusedWidget) {
    CurrentFocusedWidget->Submit();
  }
}

void UIManager::Cancel() {
  if (bTextInputActive) {
    return;
  }

  if (CurrentFocusedWidget) {
    CurrentFocusedWidget->Cancel();
  }
}
