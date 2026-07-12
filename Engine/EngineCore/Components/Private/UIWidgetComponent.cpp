#include "UIWidgetComponent.h"

#include <cmath>

#include "EngineDefine.h"
#include "UIVerticalBoxComponent.h"

MUIWidgetComponent::MUIWidgetComponent() {}

void MUIWidgetComponent::SetZOrderOffset(int offset) {
  ZOrderOffset = offset;
  for (auto* child : GetChildComponents()) {
    if (auto* childWidget = dynamic_cast<MUIWidgetComponent*>(child)) {
      childWidget->SetZOrderOffset(offset);
    }
  }
}

int MUIWidgetComponent::GetFinalPriority() const { return BasePriority + ZOrderOffset; }

void MUIWidgetComponent::SetAnchor(EUIAnchor anchor) {
  Anchor = anchor;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetPivot(const FVector2D& pivot) {
  Pivot = pivot;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetWidgetSize(const FVector2D& size) {
  WidgetSize = size;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetAnchoredPosition(const FVector2D& pos) {
  AnchoredPosition = pos;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::OnUpdate(float DeltaTime) {
  MSceneComponent::OnUpdate(DeltaTime);
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::UpdateAnchoredLocation() {
  FVector2D parentSize = {static_cast<float>(VirtualWidth), static_cast<float>(VirtualHeight)};
  FVector2D parentTopLeft = FVector2D::ZeroVector;

  if (auto parentWidget = dynamic_cast<MUIWidgetComponent*>(GetParentComponent())) {
    parentSize = parentWidget->GetWidgetSize();
    parentTopLeft = {-parentSize.X * 0.5f, -parentSize.Y * 0.5f};
  }

  FVector2D anchorOffset = FVector2D::ZeroVector;
  switch (Anchor) {
    case EUIAnchor::TopLeft:
      anchorOffset = {0.0f, 0.0f};
      break;
    case EUIAnchor::TopCenter:
      anchorOffset = {parentSize.X * 0.5f, 0.0f};
      break;
    case EUIAnchor::TopRight:
      anchorOffset = {parentSize.X, 0.0f};
      break;
    case EUIAnchor::MiddleLeft:
      anchorOffset = {0.0f, parentSize.Y * 0.5f};
      break;
    case EUIAnchor::MiddleCenter:
      anchorOffset = {parentSize.X * 0.5f, parentSize.Y * 0.5f};
      break;
    case EUIAnchor::MiddleRight:
      anchorOffset = {parentSize.X, parentSize.Y * 0.5f};
      break;
    case EUIAnchor::BottomLeft:
      anchorOffset = {0.0f, parentSize.Y};
      break;
    case EUIAnchor::BottomCenter:
      anchorOffset = {parentSize.X * 0.5f, parentSize.Y};
      break;
    case EUIAnchor::BottomRight:
      anchorOffset = {parentSize.X, parentSize.Y};
      break;
  }

  FVector2D pivotOffset = {WidgetSize.X * Pivot.X, WidgetSize.Y * Pivot.Y};

  FVector2D topLeft = parentTopLeft + anchorOffset + AnchoredPosition - pivotOffset;

  FVector2D newRelativeLoc = {topLeft.X + WidgetSize.X * 0.5f, topLeft.Y + WidgetSize.Y * 0.5f};

  if (std::abs(newRelativeLoc.X - GetRelativeLocation().X) > 0.001f ||
      std::abs(newRelativeLoc.Y - GetRelativeLocation().Y) > 0.001f) {
    SetRelativeLocation(newRelativeLoc);
  }
}

void MUIWidgetComponent::SetVisibility(bool bNewVisibility) {
  MSceneComponent::SetVisibility(bNewVisibility);
  // 親がVerticalBoxの場合、可視性変化による再レイアウトを要求する
  if (auto* parentBox = dynamic_cast<MUIVerticalBoxComponent*>(GetParentComponent())) {
    parentBox->MarkLayoutDirty();
  }
}
