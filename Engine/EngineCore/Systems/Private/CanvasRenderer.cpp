#include "CanvasRenderer.h"

#include <algorithm>

#include "BroccoliRaylib.h"
#include "RaylibResourceBridge.h"

namespace {
constexpr float CanvasStrokeThickness = 2.0f;

Color ToCanvasColor(const FColor& Value) { return {Value.R, Value.G, Value.B, Value.A}; }
}  // namespace

bool FCanvasRenderer::BeginDraw(int Handle) {
  if (!BeginRaylibRenderTexture(Handle)) return false;
  rlSetBlendFactorsSeparate(
      RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD
  );
  BeginBlendMode(BLEND_CUSTOM_SEPARATE);
  return true;
}

void FCanvasRenderer::EndDraw() {
  EndBlendMode();
  EndRaylibRenderTexture();
}

void FCanvasRenderer::Clear(const FColor& Color) { ClearBackground(ToCanvasColor(Color)); }

void FCanvasRenderer::DrawPixel(const FVector2D& Position, const FColor& Color) {
  ::DrawPixel(static_cast<int>(Position.X), static_cast<int>(Position.Y), ToCanvasColor(Color));
}

void FCanvasRenderer::DrawLine(
    const FVector2D& Start, const FVector2D& End, const FColor& Color, int Thickness
) {
  DrawLineEx(
      {Start.X, Start.Y},
      {End.X, End.Y},
      static_cast<float>((std::max)(2, Thickness)),
      ToCanvasColor(Color)
  );
}

void FCanvasRenderer::DrawCircle(
    const FVector2D& Center, float Radius, const FColor& Color, bool Fill
) {
  const Vector2 CenterPoint{Center.X, Center.Y};
  if (Fill) {
    DrawCircleV(CenterPoint, Radius, ToCanvasColor(Color));
  } else {
    DrawRing(
        CenterPoint,
        (std::max)(0.0f, Radius - CanvasStrokeThickness * 0.5f),
        Radius + CanvasStrokeThickness * 0.5f,
        0.0f,
        360.0f,
        0,
        ToCanvasColor(Color)
    );
  }
}

void FCanvasRenderer::DrawBox(
    const FVector2D& TopLeft, const FVector2D& Size, const FColor& Color, bool Fill
) {
  const Rectangle Bounds{TopLeft.X, TopLeft.Y, Size.X, Size.Y};
  if (Fill) {
    DrawRectangleRec(Bounds, ToCanvasColor(Color));
  } else {
    DrawRectangleLinesEx(Bounds, CanvasStrokeThickness, ToCanvasColor(Color));
  }
}
