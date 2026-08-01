#include "CanvasComponent.h"

#include <algorithm>
#include <cmath>

#include "BroccoliRaylib.h"
#include "Systems/Private/RaylibResourceBridge.h"

namespace {
constexpr float DefaultStrokeThickness = 2.0f;

Color ToRaylibColor(const FColor& Value) { return {Value.R, Value.G, Value.B, Value.A}; }
}  // namespace

MCanvasComponent::MCanvasComponent() { SetRenderSettings(0, RenderSpace::World); }

MCanvasComponent::~MCanvasComponent() { ReleaseCanvas(); }

bool MCanvasComponent::CreateCanvas(int InWidth, int InHeight, bool UseAlpha) {
  ReleaseCanvas();
  if (InWidth <= 0 || InHeight <= 0) return false;

  CanvasHandle = CreateRaylibRenderTexture(InWidth, InHeight);
  if (CanvasHandle == 0) return false;

  Width = InWidth;
  Height = InHeight;
  SubmitGraph(CanvasHandle);

  if (!UseAlpha && BeginDrawing()) {
    Clear(FColor::Black);
    EndDrawing();
  }
  return true;
}

void MCanvasComponent::ReleaseCanvas() {
  if (Drawing) EndDrawing();
  if (CanvasHandle != 0) {
    ReleaseRaylibTexture(CanvasHandle);
    CanvasHandle = 0;
  }
  Width = 0;
  Height = 0;
}

bool MCanvasComponent::BeginDrawing() {
  if (CanvasHandle == 0 || Drawing) return false;
  Drawing = BeginRaylibRenderTexture(CanvasHandle);
  if (Drawing) {
    rlSetBlendFactorsSeparate(
        RL_SRC_ALPHA,
        RL_ONE_MINUS_SRC_ALPHA,
        RL_ONE,
        RL_ONE_MINUS_SRC_ALPHA,
        RL_FUNC_ADD,
        RL_FUNC_ADD
    );
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
  }
  return Drawing;
}

void MCanvasComponent::EndDrawing() {
  if (!Drawing) return;
  EndBlendMode();
  EndRaylibRenderTexture();
  Drawing = false;
}

void MCanvasComponent::Clear(const FColor& ColorValue) {
  if (!Drawing) return;
  ClearBackground(ToRaylibColor(ColorValue));
}

void MCanvasComponent::DrawPixel(const FVector2D& LocalPosition, const FColor& ColorValue) {
  if (!Drawing) return;
  ::DrawPixel(
      static_cast<int>(LocalPosition.X),
      static_cast<int>(LocalPosition.Y),
      ToRaylibColor(ColorValue)
  );
}

void MCanvasComponent::DrawLine(
    const FVector2D& LocalStart, const FVector2D& LocalEnd, const FColor& ColorValue, int Thickness
) {
  if (!Drawing) return;
  DrawLineEx(
      {LocalStart.X, LocalStart.Y},
      {LocalEnd.X, LocalEnd.Y},
      static_cast<float>((std::max)(2, Thickness)),
      ToRaylibColor(ColorValue)
  );
}

void MCanvasComponent::DrawCircle(
    const FVector2D& LocalCenter, float Radius, const FColor& ColorValue, bool Fill
) {
  if (!Drawing) return;
  const Vector2 Center{LocalCenter.X, LocalCenter.Y};
  if (Fill) {
    DrawCircleV(Center, Radius, ToRaylibColor(ColorValue));
  } else {
    DrawRing(
        Center,
        (std::max)(0.0f, Radius - DefaultStrokeThickness * 0.5f),
        Radius + DefaultStrokeThickness * 0.5f,
        0.0f,
        360.0f,
        0,
        ToRaylibColor(ColorValue)
    );
  }
}

void MCanvasComponent::DrawBox(
    const FVector2D& LocalTopLeft, const FVector2D& Size, const FColor& ColorValue, bool Fill
) {
  if (!Drawing) return;
  const Rectangle Bounds{LocalTopLeft.X, LocalTopLeft.Y, Size.X, Size.Y};
  if (Fill) {
    DrawRectangleRec(Bounds, ToRaylibColor(ColorValue));
  } else {
    DrawRectangleLinesEx(Bounds, DefaultStrokeThickness, ToRaylibColor(ColorValue));
  }
}

FVector2D MCanvasComponent::WorldToCanvasLocal(const FVector2D& WorldPosition) const {
  if (Width <= 0 || Height <= 0) return FVector2D::ZeroVector();

  const FVector2D CanvasWorldPosition = GetWorldLocation();
  const FRotator CanvasWorldRotation = GetWorldRotation();
  const FScale CanvasWorldScale = GetWorldScale();
  if (std::abs(CanvasWorldScale.Scale) < 1e-6f) return FVector2D::ZeroVector();

  const float Radians = UMath::DegToRad(CanvasWorldRotation.Rotation);
  const float Cosine = std::cos(Radians);
  const float Sine = std::sin(Radians);
  const float DifferenceX = WorldPosition.X - CanvasWorldPosition.X;
  const float DifferenceY = WorldPosition.Y - CanvasWorldPosition.Y;
  const float LocalX = (DifferenceX * Cosine + DifferenceY * Sine) / CanvasWorldScale.Scale;
  const float LocalY = (-DifferenceX * Sine + DifferenceY * Cosine) / CanvasWorldScale.Scale;
  return {LocalX + Width * 0.5f, LocalY + Height * 0.5f};
}
