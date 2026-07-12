#include "UIVerticalBoxComponent.h"

#include <algorithm>
#include <vector>

struct MUIVerticalBoxComponent::Impl {
  std::vector<MUIWidgetComponent*> ListItems;
  float Spacing = 10.0f;
  bool bAutoResize = true;
  bool bNeedsLayoutUpdate = false;
};

MUIVerticalBoxComponent::MUIVerticalBoxComponent() : ImplPtr(new Impl()) {}

MUIVerticalBoxComponent::~MUIVerticalBoxComponent() { delete ImplPtr; }

void MUIVerticalBoxComponent::MarkLayoutDirty() { ImplPtr->bNeedsLayoutUpdate = true; }

void MUIVerticalBoxComponent::AddItem(MUIWidgetComponent* Item) {
  if (Item == nullptr) {
    return;
  }

  if (std::find(ImplPtr->ListItems.begin(), ImplPtr->ListItems.end(), Item) != ImplPtr->ListItems.end()) {
    return;
  }

  Item->AttachToComponent(this);
  ImplPtr->ListItems.push_back(Item);
  ImplPtr->bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::RemoveItem(MUIWidgetComponent* Item) {
  const auto it = std::find(ImplPtr->ListItems.begin(), ImplPtr->ListItems.end(), Item);
  if (it == ImplPtr->ListItems.end()) {
    return;
  }

  (*it)->AttachToComponent(nullptr);
  ImplPtr->ListItems.erase(it);
  ImplPtr->bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::ClearItems() {
  for (MUIWidgetComponent* Item : ImplPtr->ListItems) {
    if (Item != nullptr) {
      Item->AttachToComponent(nullptr);
    }
  }

  ImplPtr->ListItems.clear();
  ImplPtr->bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::SetSpacing(float InSpacing) {
  ImplPtr->Spacing = InSpacing;
  ImplPtr->bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::SetAutoResize(bool bInAutoResize) {
  ImplPtr->bAutoResize = bInAutoResize;
  ImplPtr->bNeedsLayoutUpdate = true;
}

void MUIVerticalBoxComponent::OnUpdate(float DeltaTime) {
  MUIWidgetComponent::OnUpdate(DeltaTime);

  if (!ImplPtr->bNeedsLayoutUpdate) {
    return;
  }

  UpdateLayout();
  ImplPtr->bNeedsLayoutUpdate = false;
}

void MUIVerticalBoxComponent::UpdateLayout() {
  const FVector2D currentSize = GetWidgetSize();
  const float startY = 0.0f;
  float currentY = startY;
  float maxItemWidth = 0.0f;
  bool bPlacedAnyItem = false;

  for (MUIWidgetComponent* Item : ImplPtr->ListItems) {
    if (Item == nullptr || !Item->IsVisible()) {
      continue;
    }

    const FVector2D itemSize = Item->GetWidgetSize();
    const FVector2D itemPivot = Item->GetPivot();

    Item->SetAnchor(EUIAnchor::TopCenter);
    Item->SetAnchoredPosition({0.0f, currentY + itemSize.Y * itemPivot.Y});

    currentY += itemSize.Y + ImplPtr->Spacing;
    maxItemWidth = std::max(maxItemWidth, itemSize.X);
    bPlacedAnyItem = true;
  }

  if (ImplPtr->bAutoResize) {
    const float totalHeight = bPlacedAnyItem ? currentY - startY - ImplPtr->Spacing : 0.0f;
    SetWidgetSize({std::max(currentSize.X, maxItemWidth), totalHeight});
  }
}

void MUIVerticalBoxComponent::BuildNavigation() {
  MUIButtonComponent* previousButton = nullptr;

  for (MUIWidgetComponent* Item : ImplPtr->ListItems) {
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
  for (MUIWidgetComponent* Item : ImplPtr->ListItems) {
    auto* currentButton = dynamic_cast<MUIButtonComponent*>(Item);
    if (currentButton != nullptr) {
      currentButton->Navigation.Left = LeftButton;
    }
  }
}

void MUIVerticalBoxComponent::SetNavigationRight(MUIButtonComponent* RightButton) {
  for (MUIWidgetComponent* Item : ImplPtr->ListItems) {
    auto* currentButton = dynamic_cast<MUIButtonComponent*>(Item);
    if (currentButton != nullptr) {
      currentButton->Navigation.Right = RightButton;
    }
  }
}
