#include "CanvasComponent.h"

#include <algorithm>
#include <cmath>

#include "ResourceManager.h"

MCanvasComponent::MCanvasComponent() { SetRenderSettings(0, RenderSpace::World); }

MCanvasComponent::~MCanvasComponent() { ReleaseCanvas(); }

bool MCanvasComponent::CreateCanvas(int InWidth, int InHeight, bool UseAlpha) {
  ReleaseCanvas();
  if (InWidth <= 0 || InHeight <= 0) return false;

  CanvasHandle = ResourceManager::GetInstance().CreateCanvasTexture(InWidth, InHeight);
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
    ResourceManager::GetInstance().ReleaseCanvasTexture(CanvasHandle);
    CanvasHandle = 0;
  }
  Width = 0;
  Height = 0;
}

bool MCanvasComponent::BeginDrawing() {
  if (CanvasHandle == 0 || Drawing) return false;
  Drawing = ResourceManager::GetInstance().BeginCanvasDraw(CanvasHandle);
  return Drawing;
}

void MCanvasComponent::EndDrawing() {
  if (!Drawing) return;
  ResourceManager::GetInstance().EndCanvasDraw();
  Drawing = false;
}

void MCanvasComponent::Clear(const FColor& ColorValue) {
  if (!Drawing) return;
  ResourceManager::GetInstance().ClearCanvas(ColorValue);
}

void MCanvasComponent::DrawPixel(const FVector2D& LocalPosition, const FColor& ColorValue) {
  if (!Drawing) return;
  ResourceManager::GetInstance().DrawCanvasPixel(LocalPosition, ColorValue);
}

void MCanvasComponent::DrawLine(
    const FVector2D& LocalStart, const FVector2D& LocalEnd, const FColor& ColorValue, int Thickness
) {
  if (!Drawing) return;
  ResourceManager::GetInstance().DrawCanvasLine(LocalStart, LocalEnd, ColorValue, Thickness);
}

void MCanvasComponent::DrawCircle(
    const FVector2D& LocalCenter, float Radius, const FColor& ColorValue, bool Fill
) {
  if (!Drawing) return;
  ResourceManager::GetInstance().DrawCanvasCircle(LocalCenter, Radius, ColorValue, Fill);
}

void MCanvasComponent::DrawBox(
    const FVector2D& LocalTopLeft, const FVector2D& Size, const FColor& ColorValue, bool Fill
) {
  if (!Drawing) return;
  ResourceManager::GetInstance().DrawCanvasBox(LocalTopLeft, Size, ColorValue, Fill);
}

FVector2D MCanvasComponent::WorldToCanvasLocal(const FVector2D& WorldPosition) const {
  if (Width <= 0 || Height <= 0) return FVector2D::ZeroVector();

  const FVector2D CanvasWorldPosition = GetWorldLocation2D();
  const FRotator CanvasWorldRotation = GetWorldRotation2D();
  const FScale CanvasWorldScale = GetWorldScale2D();
  if (std::abs(CanvasWorldScale.Scale) < 1e-6f) return FVector2D::ZeroVector();

  const float Radians = UMath::DegToRad(CanvasWorldRotation.Rotation);
  const float Cosine = std::cos(Radians);
  const float Sine = std::sin(Radians);
  const float DifferenceX = WorldPosition.X - CanvasWorldPosition.X;
  const float DifferenceY = WorldPosition.Y - CanvasWorldPosition.Y;
  const float LocalX = (DifferenceX * Cosine + DifferenceY * Sine) / CanvasWorldScale.Scale;
  const float LocalY = -(-DifferenceX * Sine + DifferenceY * Cosine) / CanvasWorldScale.Scale;
  return {LocalX + Width * 0.5f, LocalY + Height * 0.5f};
}
