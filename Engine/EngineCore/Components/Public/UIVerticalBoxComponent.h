#pragma once
#include "BroccoliEngineAPI.h"

#include <vector>

#include "UIButtonComponent.h"
#include "UIWidgetComponent.h"

class BROCCOLI_ENGINE_API MUIVerticalBoxComponent : public MUIWidgetComponent {
 public:
  MUIVerticalBoxComponent();

  void AddItem(MUIWidgetComponent* Item);
  void RemoveItem(MUIWidgetComponent* Item);
  void ClearItems();

  void SetSpacing(float InSpacing);
  void SetAutoResize(bool bInAutoResize);

  void BuildNavigation();
  void SetNavigationLeft(MUIButtonComponent* LeftButton);
  void SetNavigationRight(MUIButtonComponent* RightButton);
  void OnUpdate(float DeltaTime) override;
  // 子要素の可視性変更などにより外部から再レイアウトを要求する
  void MarkLayoutDirty() { bNeedsLayoutUpdate = true; }

 protected:
  void UpdateLayout();

 private:
  std::vector<MUIWidgetComponent*> ListItems;
  float Spacing = 10.0f;
  bool bAutoResize = true;
  bool bNeedsLayoutUpdate = false;
};
