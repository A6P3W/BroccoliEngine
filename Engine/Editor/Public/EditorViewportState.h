#pragma once

#include "UMath.h"

struct FEditorViewportState {
  void* RenderTexture = nullptr;
  FVector2D RequestedRenderSize = FVector2D::ZeroVector();
  FVector2D RenderTargetSize = FVector2D::ZeroVector();
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

  bool HasValidImage() const {
    return ImageSize.X > 0.0f && ImageSize.Y > 0.0f && RenderTargetSize.X > 0.0f &&
           RenderTargetSize.Y > 0.0f;
  }

  bool ContainsScreenPoint(const FVector2D& ScreenPoint) const {
    return HasValidImage() && ScreenPoint.X >= ImagePosition.X &&
           ScreenPoint.Y >= ImagePosition.Y && ScreenPoint.X < ImagePosition.X + ImageSize.X &&
           ScreenPoint.Y < ImagePosition.Y + ImageSize.Y;
  }

  bool ScreenToRenderTarget(
      const FVector2D& ScreenPoint, FVector2D& OutRenderTargetPoint, bool RequireInside = true
  ) const {
    if (!HasValidImage() || (RequireInside && !ContainsScreenPoint(ScreenPoint))) return false;

    OutRenderTargetPoint = {
        (ScreenPoint.X - ImagePosition.X) * RenderTargetSize.X / ImageSize.X,
        (ScreenPoint.Y - ImagePosition.Y) * RenderTargetSize.Y / ImageSize.Y,
    };
    return true;
  }

  FVector2D ScreenDeltaToRenderTarget(const FVector2D& ScreenDelta) const {
    if (!HasValidImage()) return FVector2D::ZeroVector();
    return {
        ScreenDelta.X * RenderTargetSize.X / ImageSize.X,
        ScreenDelta.Y * RenderTargetSize.Y / ImageSize.Y,
    };
  }
};
