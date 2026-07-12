#include "UIWidgetComponent.h"

#include <cmath>

#include "EngineDefine.h"
#include "UIVerticalBoxComponent.h"

struct MUIWidgetComponent::Impl {
  int BasePriority = 0;
  int ZOrderOffset = 0;
  EUIAnchor Anchor = EUIAnchor::MiddleCenter;
  FVector2D Pivot = FVector2D(0.5f, 0.5f);
  FVector2D WidgetSize = FVector2D::ZeroVector();
  FVector2D AnchoredPosition = FVector2D::ZeroVector();
};

MUIWidgetComponent::MUIWidgetComponent() : ImplPtr(new Impl()) {}

MUIWidgetComponent::~MUIWidgetComponent() { delete ImplPtr; }

void MUIWidgetComponent::SetZOrderOffset(int offset) {
  ImplPtr->ZOrderOffset = offset;
  for (auto* child : GetChildComponents()) {
    if (auto* childWidget = dynamic_cast<MUIWidgetComponent*>(child)) {
      childWidget->SetZOrderOffset(offset);
    }
  }
}

int MUIWidgetComponent::GetFinalPriority() const { return ImplPtr->BasePriority + ImplPtr->ZOrderOffset; }

void MUIWidgetComponent::SetAnchor(EUIAnchor anchor) {
  ImplPtr->Anchor = anchor;
  UpdateAnchoredLocation();
}

EUIAnchor MUIWidgetComponent::GetAnchor() const { return ImplPtr->Anchor; }

void MUIWidgetComponent::SetPivot(const FVector2D& pivot) {
  ImplPtr->Pivot = pivot;
  UpdateAnchoredLocation();
}

FVector2D MUIWidgetComponent::GetPivot() const { return ImplPtr->Pivot; }

void MUIWidgetComponent::SetWidgetSize(const FVector2D& size) {
  ImplPtr->WidgetSize = size;
  UpdateAnchoredLocation();
}

FVector2D MUIWidgetComponent::GetWidgetSize() const { return ImplPtr->WidgetSize; }

void MUIWidgetComponent::SetAnchoredPosition(const FVector2D& pos) {
  ImplPtr->AnchoredPosition = pos;
  UpdateAnchoredLocation();
}

FVector2D MUIWidgetComponent::GetAnchoredPosition() const { return ImplPtr->AnchoredPosition; }

void MUIWidgetComponent::OnUpdate(float DeltaTime) {
  MSceneComponent::OnUpdate(DeltaTime);
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::UpdateAnchoredLocation() {
  FVector2D parentSize = {static_cast<float>(VirtualWidth), static_cast<float>(VirtualHeight)};
  FVector2D parentTopLeft = FVector2D::ZeroVector();

  if (auto parentWidget = dynamic_cast<MUIWidgetComponent*>(GetParentComponent())) {
    parentSize = parentWidget->GetWidgetSize();
    parentTopLeft = {-parentSize.X * 0.5f, -parentSize.Y * 0.5f};
  }

  FVector2D anchorOffset = FVector2D::ZeroVector();
  switch (ImplPtr->Anchor) {
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

  FVector2D pivotOffset = {
      ImplPtr->WidgetSize.X * ImplPtr->Pivot.X, ImplPtr->WidgetSize.Y * ImplPtr->Pivot.Y
  };

  FVector2D topLeft = parentTopLeft + anchorOffset + ImplPtr->AnchoredPosition - pivotOffset;

  FVector2D newRelativeLoc = {
      topLeft.X + ImplPtr->WidgetSize.X * 0.5f, topLeft.Y + ImplPtr->WidgetSize.Y * 0.5f
  };

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