#include "GridLine.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "UMath.h"
AGridLine::AGridLine() { bEditorActor = true; }

void AGridLine::BeginPlay() {
  CollisionCellSize = GetWorld()->GetCollisionSystem()->GetCollisionCellSize();
}

void AGridLine::OnUpdate(float DeltaTime) { SimpleDraw(CollisionCellSize, FColor{255, 255, 255}); }

void AGridLine::SimpleDraw(float CellSize, FColor Color) {
  auto Cam = RenderSystem::GetInstance().GetCamera();
  if (!Cam) return;

  FVector2D CamPos = Cam->GetWorldLocation();
  float Fov = Cam->GetFOV();

  float MinScreenSpacing = 50.0f;
  int SkipFactor = 1;

  while ((CellSize * Fov * SkipFactor) < MinScreenSpacing) {
    SkipFactor *= 2;
  }

  float VisualCellSize = CellSize * SkipFactor;

  const float ScreenDiagonal =
      std::sqrt(static_cast<float>(VirtualWidth * VirtualWidth + VirtualHeight * VirtualHeight));
  float WorldRadius = (ScreenDiagonal * 0.5f) / Fov;

  float StartX = std::floor((CamPos.X - WorldRadius) / VisualCellSize) * VisualCellSize;
  float EndX = std::ceil((CamPos.X + WorldRadius) / VisualCellSize) * VisualCellSize;
  float StartY = std::floor((CamPos.Y - WorldRadius) / VisualCellSize) * VisualCellSize;
  float EndY = std::ceil((CamPos.Y + WorldRadius) / VisualCellSize) * VisualCellSize;

  const float ScreenSpacing = VisualCellSize * Fov;
  const float AlphaT =
      std::clamp((ScreenSpacing - MinScreenSpacing) / MinScreenSpacing, 0.0f, 1.0f);
  Color.A = static_cast<uint8_t>(std::lerp(100.0f, 200.0f, AlphaT));

  // 垂直線
  for (float X = StartX; X <= EndX; X += VisualCellSize) {
    RenderSystem::GetInstance().SubmitLine({X, StartY}, {X, EndY}, Color, RenderSpace::World, 999);
  }
  // 水平線
  for (float Y = StartY; Y <= EndY; Y += VisualCellSize) {
    RenderSystem::GetInstance().SubmitLine({StartX, Y}, {EndX, Y}, Color, RenderSpace::World, 999);
  }

  RenderSystem::GetInstance().SubmitLine(
      {0, StartY}, {0, EndY}, FColor{255, 100, 100, 220}, RenderSpace::World, 999
  );
  RenderSystem::GetInstance().SubmitLine(
      {StartX, 0}, {EndX, 0}, FColor{255, 100, 100, 220}, RenderSpace::World, 999
  );
}
