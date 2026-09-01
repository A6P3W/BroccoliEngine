#pragma once

#include "EngineDefine.h"
#include "UMath.h"

struct FEditorViewportState {
  void* RenderTexture = nullptr;
  FVector2D ImagePosition = FVector2D::ZeroVector();
  FVector2D ImageSize = FVector2D::ZeroVector();
  bool Hovered = false;
  bool Focused = false;

  void ResetFrameState() {
    ImagePosition = FVector2D::ZeroVector();
    ImageSize = FVector2D::ZeroVector();
    Hovered = false;
    Focused = false;
  }

  bool HasValidImage() const { return ImageSize.X > 0.0f && ImageSize.Y > 0.0f; }

  bool ContainsScreenPoint(const FVector2D& ScreenPoint) const {
    return HasValidImage() && ScreenPoint.X >= ImagePosition.X &&
           ScreenPoint.Y >= ImagePosition.Y && ScreenPoint.X < ImagePosition.X + ImageSize.X &&
           ScreenPoint.Y < ImagePosition.Y + ImageSize.Y;
  }

  bool ScreenToVirtual(
      const FVector2D& ScreenPoint, FVector2D& OutVirtualPoint, bool RequireInside = true
  ) const {
    if (!HasValidImage() || (RequireInside && !ContainsScreenPoint(ScreenPoint))) return false;

    OutVirtualPoint = {
        (ScreenPoint.X - ImagePosition.X) * static_cast<float>(VirtualWidth) / ImageSize.X,
        (ScreenPoint.Y - ImagePosition.Y) * static_cast<float>(VirtualHeight) / ImageSize.Y,
    };
    return true;
  }

  FVector2D ScreenDeltaToVirtual(const FVector2D& ScreenDelta) const {
    if (!HasValidImage()) return FVector2D::ZeroVector();
    return {
        ScreenDelta.X * static_cast<float>(VirtualWidth) / ImageSize.X,
        ScreenDelta.Y * static_cast<float>(VirtualHeight) / ImageSize.Y,
    };
  }
};
