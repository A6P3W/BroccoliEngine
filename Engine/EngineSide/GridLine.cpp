#include "GridLine.h"

#include <DxLib.h>

#include <cmath>
#include <format>
#include <string>

#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "UMath.h"
AGridLine::AGridLine() { bEditorActor = true; }

void AGridLine::BeginPlay() {
  CollisionCellSize = GetWorld()->GetCollisionSystem()->GetCollisionCellSize();
}

void AGridLine::OnUpdate(float DeltaTime) {
  SimpleDraw(CollisionCellSize, GetColor(255, 255, 255));
}

void AGridLine::SimpleDraw(float cellSize, int color) {
  auto Cam = RenderSystem::GetInstance().GetCamera();
  if (!Cam) return;

  FVector2D CamPos = Cam->GetWorldLocation();
  float fov = Cam->GetFOV();

  float minScreenSpacing = 50.0f;
  int skipFactor = 1;

  while ((cellSize * fov * skipFactor) < minScreenSpacing) {
    skipFactor *= 2;
  }

  float visualCellSize = cellSize * skipFactor;

  int screenW, screenH;
  GetDrawScreenSize(&screenW, &screenH);
  float screenDiagonal = std::sqrt((float)screenW * screenW + (float)screenH * screenH);
  float worldRadius = (screenDiagonal * 0.5f) / fov;

  float startX = std::floor((CamPos.X - worldRadius) / visualCellSize) * visualCellSize;
  float endX = std::ceil((CamPos.X + worldRadius) / visualCellSize) * visualCellSize;
  float startY = std::floor((CamPos.Y - worldRadius) / visualCellSize) * visualCellSize;
  float endY = std::ceil((CamPos.Y + worldRadius) / visualCellSize) * visualCellSize;

  int alpha = (skipFactor == 1) ? 60 : 100;

  // 垂直線
  for (float x = startX; x <= endX; x += visualCellSize) {
    RenderSystem::GetInstance().SubmitLine(
        {x, startY}, {x, endY}, color, RenderSpace::World, 999, alpha
    );
  }
  // 水平線
  for (float y = startY; y <= endY; y += visualCellSize) {
    RenderSystem::GetInstance().SubmitLine(
        {startX, y}, {endX, y}, color, RenderSpace::World, 999, alpha
    );
  }

  RenderSystem::GetInstance().SubmitLine(
      {0, startY}, {0, endY}, GetColor(255, 100, 100), RenderSpace::World, 999, 120
  );
  RenderSystem::GetInstance().SubmitLine(
      {startX, 0}, {endX, 0}, GetColor(255, 100, 100), RenderSpace::World, 999, 120
  );
}
