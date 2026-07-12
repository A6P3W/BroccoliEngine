#pragma once
#include "BroccoliEngineAPI.h"
#include "SceneComponent.h"
#include "UMath.h"

enum class EUIAnchor {
  TopLeft,
  TopCenter,
  TopRight,
  MiddleLeft,
  MiddleCenter,
  MiddleRight,
  BottomLeft,
  BottomCenter,
  BottomRight
};

class BROCCOLI_ENGINE_API MUIWidgetComponent : public MSceneComponent {
 public:
  MUIWidgetComponent();
  ~MUIWidgetComponent() override;

  void SetZOrderOffset(int offset);
  int GetFinalPriority() const;

  void SetAnchor(EUIAnchor anchor);
  EUIAnchor GetAnchor() const;

  void SetPivot(const FVector2D& pivot);
  FVector2D GetPivot() const;

  void SetWidgetSize(const FVector2D& size);
  FVector2D GetWidgetSize() const;

  void SetAnchoredPosition(const FVector2D& pos);
  FVector2D GetAnchoredPosition() const;

  void OnUpdate(float DeltaTime) override;
  void SetVisibility(bool bNewVisibility) override;

 protected:
  void UpdateAnchoredLocation();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};