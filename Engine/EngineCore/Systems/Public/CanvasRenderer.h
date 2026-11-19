#pragma once

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UMath.h"

// 2D drawing into an offscreen canvas texture created by ResourceManager::CreateCanvasTexture.
// All draw calls must be made between BeginDraw and EndDraw.
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
