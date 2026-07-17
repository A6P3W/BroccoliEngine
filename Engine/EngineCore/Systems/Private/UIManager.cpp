#include "UIManager.h"

UIManager* UIManager::GetInstance() {
  static UIManager Instance;
  return &Instance;
}

#include <algorithm>

#include "Actor.h"
#include "EnhancedInputComponent.h"
#include "Log.h"
#include "WidgetBase.h"

struct UIManager::Impl {
  AWidgetBase* CurrentFocusedWidget = nullptr;
  std::vector<AWidgetBase*> ActiveWidgets;
  bool TextInputActive = false;
};

UIManager::UIManager() : ImplPtr(new Impl()) {}

UIManager::~UIManager() {
  delete ImplPtr;
}

void UIManager::SetTextInputActive(bool bActive) { ImplPtr->TextInputActive = bActive; }

bool UIManager::IsTextInputActive() const { return ImplPtr->TextInputActive; }

void UIManager::SetFocusedWidget(AWidgetBase* Widget) { ImplPtr->CurrentFocusedWidget = Widget; }

AWidgetBase* UIManager::GetFocusedWidget() const { return ImplPtr->CurrentFocusedWidget; }
void UIManager::RefreshZOrder() {
  int currentOffset = 0;
  for (AWidgetBase* widget : ImplPtr->ActiveWidgets) {
    if (widget) {
      widget->SetZOrderOffset(currentOffset);
      currentOffset += 100;
    }
  }
}

void UIManager::AddWidget(AWidgetBase* Widget) {
  if (!Widget) return;

  ImplPtr->ActiveWidgets.push_back(Widget);
  Widget->OnOpened();

  RefreshZOrder();
}

void UIManager::RemoveWidget(AWidgetBase* Widget) {
  if (!Widget) return;

  if (ImplPtr->CurrentFocusedWidget == Widget) {
    ImplPtr->CurrentFocusedWidget = nullptr;
    ImplPtr->TextInputActive = false;
  }

  auto it = std::find(ImplPtr->ActiveWidgets.begin(), ImplPtr->ActiveWidgets.end(), Widget);
  if (it != ImplPtr->ActiveWidgets.end()) {
    (*it)->OnClosed();
    (*it)->Destroy();
    ImplPtr->ActiveWidgets.erase(it);

    RefreshZOrder();
  }
}

void UIManager::Navigate(const FInputActionValue& Value) {
  if (ImplPtr->TextInputActive) {
    return;
  }

  if (ImplPtr->CurrentFocusedWidget) {
    ImplPtr->CurrentFocusedWidget->Navigate(Value);
  }
}

void UIManager::Submit() {
  if (ImplPtr->TextInputActive) {
    return;
  }

  if (ImplPtr->CurrentFocusedWidget) {
    ImplPtr->CurrentFocusedWidget->Submit();
  }
}

void UIManager::Cancel() {
  if (ImplPtr->TextInputActive) {
    return;
  }

  if (ImplPtr->CurrentFocusedWidget) {
    ImplPtr->CurrentFocusedWidget->Cancel();
  }
}
