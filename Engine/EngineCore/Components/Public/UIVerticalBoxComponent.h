#pragma once
#include "BroccoliEngineAPI.h"

#include "UIButtonComponent.h"
#include "UIWidgetComponent.h"

class BROCCOLI_ENGINE_API MUIVerticalBoxComponent : public MUIWidgetComponent {
 public:
  MUIVerticalBoxComponent();
  ~MUIVerticalBoxComponent() override;

  void AddItem(MUIWidgetComponent* Item);
  void RemoveItem(MUIWidgetComponent* Item);
  void ClearItems();

  void SetSpacing(float InSpacing);
  void SetAutoResize(bool bInAutoResize);

  void BuildNavigation();
  void SetNavigationLeft(MUIButtonComponent* LeftButton);
  void SetNavigationRight(MUIButtonComponent* RightButton);
  void OnUpdate(float DeltaTime) override;
  void MarkLayoutDirty();

 protected:
  void UpdateLayout();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};