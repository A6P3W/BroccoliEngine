#include "UIVerticalBoxComponent.h"

#include <algorithm>

MUIVerticalBoxComponent::MUIVerticalBoxComponent(int basePriority)
    : MUIWidgetComponent(basePriority) {}

void MUIVerticalBoxComponent::AddItem(MUIWidgetComponent* Item) {
  if (Item == nullptr) {
    return;
  }

  if (std::find(ListItems.begin(), ListItems.end(), Item) != ListItems.end()) {
    return;
  }

  Item->SetParentComponent(this);
  ListItems.push_back(Item);
  bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::RemoveItem(MUIWidgetComponent* Item) {
  const auto it = std::find(ListItems.begin(), ListItems.end(), Item);
  if (it == ListItems.end()) {
    return;
  }

  (*it)->SetParentComponent(nullptr);
  ListItems.erase(it);
  bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::ClearItems() {
  for (MUIWidgetComponent* Item : ListItems) {
    if (Item != nullptr) {
      Item->SetParentComponent(nullptr);
    }
  }

  ListItems.clear();
  bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::SetSpacing(float InSpacing) {
  Spacing = InSpacing;
  bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::SetAutoResize(bool bInAutoResize) {
  bAutoResize = bInAutoResize;
  bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::OnUpdate(float DeltaTime) {
  MUIWidgetComponent::OnUpdate(DeltaTime);

  if (!bNeedsLayoutUpdate) {
    return;
  }

  UpdateLayout();
  bNeedsLayoutUpdate = false;
}

void MUIVerticalBoxComponent::UpdateLayout() {
  const FVector2D currentSize = GetWidgetSize();
  const float startY = 0.0f;
  float currentY = startY;
  float maxItemWidth = 0.0f;
  bool bPlacedAnyItem = false;

  for (MUIWidgetComponent* Item : ListItems) {
    if (Item == nullptr || !Item->IsVisible()) {
      continue;
    }

    const FVector2D itemSize = Item->GetWidgetSize();
    const FVector2D itemPivot = Item->GetPivot();

    Item->SetAnchor(EUIAnchor::TopCenter);
    Item->SetAnchoredPosition({0.0f, currentY + itemSize.Y * itemPivot.Y});

    currentY += itemSize.Y + Spacing;
    maxItemWidth = std::max(maxItemWidth, itemSize.X);
    bPlacedAnyItem = true;
  }

  if (bAutoResize) {
    const float totalHeight = bPlacedAnyItem ? currentY - startY - Spacing : 0.0f;
    SetWidgetSize({std::max(currentSize.X, maxItemWidth), totalHeight});
  }
}

void MUIVerticalBoxComponent::BuildNavigation() {
  MUIButtonComponent* previousButton = nullptr;

  for (MUIWidgetComponent* Item : ListItems) {
    if (Item == nullptr || !Item->IsVisible()) {
      continue;
    }

    auto* currentButton = dynamic_cast<MUIButtonComponent*>(Item);
    if (currentButton == nullptr) {
      continue;
    }

    currentButton->Navigation.Up = previousButton;
    currentButton->Navigation.Down = nullptr;

    if (previousButton != nullptr) {
      previousButton->Navigation.Down = currentButton;
    }

    previousButton = currentButton;
  }
}

void MUIVerticalBoxComponent::SetNavigationLeft(MUIButtonComponent* LeftButton) {
  for (MUIWidgetComponent* Item : ListItems) {
    auto* currentButton = dynamic_cast<MUIButtonComponent*>(Item);
    if (currentButton != nullptr) {
      currentButton->Navigation.Left = LeftButton;
    }
  }
}

void MUIVerticalBoxComponent::SetNavigationRight(MUIButtonComponent* RightButton) {
  for (MUIWidgetComponent* Item : ListItems) {
    auto* currentButton = dynamic_cast<MUIButtonComponent*>(Item);
    if (currentButton != nullptr) {
      currentButton->Navigation.Right = RightButton;
    }
  }
}
