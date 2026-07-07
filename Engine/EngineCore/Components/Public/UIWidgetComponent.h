#pragma once
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

class MUIWidgetComponent : public MSceneComponent {
 public:
  MUIWidgetComponent(int basePriority = 0);

  void SetZOrderOffset(int offset);
  int GetFinalPriority() const;

  // アンカー（親要素のどの位置を基準とするか）を設定
  void SetAnchor(EUIAnchor anchor);
  EUIAnchor GetAnchor() const { return Anchor; }

  // ピボット（自身の要素のどの位置を原点とするか、0.0~1.0）を設定
  void SetPivot(const FVector2D& pivot);
  FVector2D GetPivot() const { return Pivot; }

  // UIのサイズを設定
  void SetWidgetSize(const FVector2D& size);
  FVector2D GetWidgetSize() const { return WidgetSize; }

  // アンカー基準点からの相対的な表示位置を設定
  void SetAnchoredPosition(const FVector2D& pos);
  FVector2D GetAnchoredPosition() const { return AnchoredPosition; }

  void OnUpdate(float DeltaTime) override;
  // 可視性変更時、親のVerticalBoxに再レイアウトを通知する
  void SetVisibility(bool bNewVisibility) override;

 protected:
  // アンカーとピボットを基に実際の相対座標(RelativeLocation)を計算
  void UpdateAnchoredLocation();

  int BasePriority = 0;
  int ZOrderOffset = 0;

  EUIAnchor Anchor = EUIAnchor::MiddleCenter;
  FVector2D Pivot = FVector2D(0.5f, 0.5f);
  FVector2D WidgetSize = FVector2D::ZeroVector;
  FVector2D AnchoredPosition = FVector2D::ZeroVector;
};
