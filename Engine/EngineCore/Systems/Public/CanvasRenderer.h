#pragma once

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UMath.h"

/// @brief ResourceManager::CreateCanvasTexture で作成したオフスクリーンキャンバステクスチャへの2D描画。
/// @brief すべての描画呼び出しは BeginDraw と EndDraw の間で行う必要がある。
class BROCCOLI_ENGINE_API FCanvasRenderer {
 public:
  static bool BeginDraw(int CanvasHandle);
  static void EndDraw();
  static void Clear(const FColor& Color);
  static void DrawPixel(const FVector2D& Position, const FColor& Color);
  static void DrawLine(
      const FVector2D& Start, const FVector2D& End, const FColor& Color, int Thickness
  );
  static void DrawCircle(const FVector2D& Center, float Radius, const FColor& Color, bool Fill);
  static void DrawBox(
      const FVector2D& TopLeft, const FVector2D& Size, const FColor& Color, bool Fill
  );
};
