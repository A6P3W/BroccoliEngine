#include "UIWidgetComponent.h"

#include <cmath>

#include "EngineDefine.h"

MUIWidgetComponent::MUIWidgetComponent(int basePriority) : m_basePriority(basePriority) {}

void MUIWidgetComponent::SetZOrderOffset(int offset) {
  m_zOrderOffset = offset;
  for (auto* child : ChildComponents) {
    if (auto* childWidget = dynamic_cast<MUIWidgetComponent*>(child)) {
      childWidget->SetZOrderOffset(offset);
    }
  }
}

int MUIWidgetComponent::GetFinalPriority() const { return m_basePriority + m_zOrderOffset; }

void MUIWidgetComponent::SetAnchor(EUIAnchor anchor) {
  m_anchor = anchor;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetPivot(const FVector2D& pivot) {
  m_pivot = pivot;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetWidgetSize(const FVector2D& size) {
  m_widgetSize = size;
  UpdateAnchoredLocation();
}

void MUIWidgetComponent::SetAnchoredPosition(const FVector2D& pos) {
  m_anchoredPosition = pos;
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
  switch (m_anchor) {
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

  FVector2D pivotOffset = {m_widgetSize.X * m_pivot.X, m_widgetSize.Y * m_pivot.Y};

  FVector2D topLeft = parentTopLeft + anchorOffset + m_anchoredPosition - pivotOffset;

  FVector2D newRelativeLoc = {topLeft.X + m_widgetSize.X * 0.5f, topLeft.Y + m_widgetSize.Y * 0.5f};

  if (std::abs(newRelativeLoc.X - GetRelativeLocation().X) > 0.001f ||
      std::abs(newRelativeLoc.Y - GetRelativeLocation().Y) > 0.001f) {
    SetRelativeLocation(newRelativeLoc);
  }
}
