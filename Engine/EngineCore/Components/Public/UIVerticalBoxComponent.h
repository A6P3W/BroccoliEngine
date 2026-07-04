#pragma once

#include <vector>

#include "UIButtonComponent.h"
#include "UIWidgetComponent.h"

class MUIVerticalBoxComponent : public MUIWidgetComponent {
 public:
  MUIVerticalBoxComponent(int basePriority = 0);

  void AddItem(MUIWidgetComponent* Item);
  void RemoveItem(MUIWidgetComponent* Item);
  void ClearItems();

  void SetSpacing(float InSpacing);
  void SetAutoResize(bool bInAutoResize);

  void BuildNavigation();
  void SetNavigationLeft(MUIButtonComponent* LeftButton);
  void SetNavigationRight(MUIButtonComponent* RightButton);
  void OnUpdate(float DeltaTime) override;

 protected:
  void UpdateLayout();

 private:
  std::vector<MUIWidgetComponent*> ListItems;
  float Spacing = 10.0f;
  bool bAutoResize = true;
  bool bNeedsLayoutUpdate = false;
};
