#include "RenderSystem.h"

#include <algorithm>
#include <cmath>

#include "CameraComponent.h"
#include "DxLib.h"
#include "EngineDefine.h"

RenderSystem::RenderSystem() {}

RenderSystem& RenderSystem::GetInstance() {
  static RenderSystem instance;
  return instance;
}

void RenderSystem::SubmitGraph(
    FVector2D Location,
    int Handle,
    FScale Scale,
    FRotator Rotation,
    RenderSpace Space,
    int Priority,
    int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = GraphData{Location, Rotation, Scale, Handle};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

void RenderSystem::SubmitCircle(
    FVector2D Location,
    float Radius,
    int Color,
    bool Fill,
    RenderSpace Space,
    int Priority,
    int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = CircleData{Location, Radius, Color, Fill};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

void RenderSystem::SubmitBox(
    FVector2D Location,
    FVector2D WidthHeight,
    FRotator Rotation,
    int Color,
    bool Fill,
    RenderSpace Space,
    int Priority,
    int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = BoxData{Location, WidthHeight, Rotation, Color, Fill};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

void RenderSystem::SubmitText(
    FVector2D Location,
    const std::string& Text,
    int Handle,
    int Color,
    RenderSpace Space,
    int Priority,
    int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = TextData{Location, Text, Color, Handle};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

void RenderSystem::SubmitLine(
    FVector2D Start, FVector2D End, int Color, RenderSpace Space, int Priority, int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = LineData{Start, End, Color};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

void RenderSystem::SubmitRectGraph(
    FVector2D Dest,
    FVector2D SrcLoc,
    FVector2D SrcSize,
    int Handle,
    RenderSpace Space,
    int Priority,
    int Alpha
) {
  RenderCommand cmd;
  cmd.common = {Priority, Alpha, Space};
  cmd.data = RectGraphData{Dest, SrcLoc, SrcSize, Handle};
  PriorityCommands[Priority].push_back(std::move(cmd));
}

// --- 座標変換 ---

FVector2D RenderSystem::WorldToScreen(const FVector2D& worldPos) const {
  FVector2D camPos = FVector2D::ZeroVector;
  float camRot = 0.0f;
  float camFOV = 1.0f;
  if (MainCamera) {
    camPos = MainCamera->GetWorldLocation();
    camRot = MainCamera->GetWorldRotation().Rotation;
    camFOV = MainCamera->GetFOV();
  }
  const float centerX = VirtualWidth * 0.5f;
  const float centerY = VirtualHeight * 0.5f;
  const float localX = worldPos.X - camPos.X;
  const float localY = worldPos.Y - camPos.Y;
  const float rad = UMath::DegToRad(-camRot);
  const float cosT = std::cos(rad), sinT = std::sin(rad);
  return {
      (localX * cosT - localY * sinT) * camFOV + centerX,
      (localX * sinT + localY * cosT) * camFOV + centerY
  };
}

FVector2D RenderSystem::ScreenToWorld(const FVector2D& screenPos) const {
  FVector2D camPos = FVector2D::ZeroVector;
  float camRot = 0.0f;
  float camFOV = 1.0f;
  if (MainCamera) {
    camPos = MainCamera->GetWorldLocation();
    camRot = MainCamera->GetWorldRotation().Rotation;
    camFOV = MainCamera->GetFOV();
  }
  const float centerX = VirtualWidth * 0.5f;
  const float centerY = VirtualHeight * 0.5f;
  const float safeFOV = (std::abs(camFOV) < 1e-6f) ? 1e-6f : camFOV;
  const float localX = (screenPos.X - centerX) / safeFOV;
  const float localY = (screenPos.Y - centerY) / safeFOV;
  const float rad = UMath::DegToRad(camRot);
  const float cosT = std::cos(rad), sinT = std::sin(rad);
  return {(localX * cosT - localY * sinT) + camPos.X, (localX * sinT + localY * cosT) + camPos.Y};
}

void RenderSystem::Draw() {
  float camRot = MainCamera ? MainCamera->GetWorldRotation().Rotation : 0.0f;
  float camFOV = MainCamera ? MainCamera->GetFOV() : 1.0f;

  int curBlend, curAlpha;
  GetDrawBlendMode(&curBlend, &curAlpha);

  for (auto& [priority, commands] : PriorityCommands) {
    for (const auto& cmd : commands) {
      // アルファ値更新
      if (cmd.common.alpha != curAlpha) {
        curAlpha = std::clamp(cmd.common.alpha, 0, 255);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, curAlpha);
      }

      std::visit(
          [&](auto&& d) {
            using T = std::decay_t<decltype(d)>;

            // --- Graph ---
            if constexpr (std::is_same_v<T, GraphData>) {
              FVector2D drawPos =
                  (cmd.common.space == RenderSpace::World) ? WorldToScreen(d.Location) : d.Location;
              float drawScale =
                  d.Scale.Scale * (cmd.common.space == RenderSpace::World ? camFOV : 1.0f);
              float drawRot =
                  UMath::DegToRad(d.Rotation.Rotation) -
                  (cmd.common.space == RenderSpace::World ? UMath::DegToRad(camRot) : 0.0f);
              DrawRotaGraphFastF(drawPos.X, drawPos.Y, drawScale, drawRot, d.Handle, TRUE);
            }
            // --- Box ---
            else if constexpr (std::is_same_v<T, BoxData>) {
              if (cmd.common.space == RenderSpace::World) {
                float rad = UMath::DegToRad(d.Rotation.Rotation);
                float cosA = std::cos(rad), sinA = std::sin(rad);
                auto GetV = [&](float lx, float ly) {
                  return WorldToScreen(
                      {d.Location.X + (lx * cosA - ly * sinA),
                       d.Location.Y + (lx * sinA + ly * cosA)}
                  );
                };
                FVector2D v1 = GetV(0, 0), v2 = GetV(d.WidthHeight.X, 0),
                          v3 = GetV(d.WidthHeight.X, d.WidthHeight.Y),
                          v4 = GetV(0, d.WidthHeight.Y);
                DrawQuadrangle(
                    static_cast<int>(v1.X),
                    static_cast<int>(v1.Y),
                    static_cast<int>(v2.X),
                    static_cast<int>(v2.Y),
                    static_cast<int>(v3.X),
                    static_cast<int>(v3.Y),
                    static_cast<int>(v4.X),
                    static_cast<int>(v4.Y),
                    d.Color,
                    d.Fill
                );
              } else {
                DrawBox(
                    static_cast<int>(d.Location.X),
                    static_cast<int>(d.Location.Y),
                    static_cast<int>(d.Location.X + d.WidthHeight.X),
                    static_cast<int>(d.Location.Y + d.WidthHeight.Y),
                    d.Color,
                    d.Fill
                );
              }
            }
            // --- Circle ---
            else if constexpr (std::is_same_v<T, CircleData>) {
              FVector2D drawPos =
                  (cmd.common.space == RenderSpace::World) ? WorldToScreen(d.Location) : d.Location;
              float drawRadius =
                  d.Radius * (cmd.common.space == RenderSpace::World ? camFOV : 1.0f);
              DrawCircle(
                  static_cast<int>(drawPos.X),
                  static_cast<int>(drawPos.Y),
                  static_cast<int>(drawRadius),
                  d.Color,
                  d.Fill
              );
            }
            // --- Text ---
            else if constexpr (std::is_same_v<T, TextData>) {
              FVector2D drawPos =
                  (cmd.common.space == RenderSpace::World) ? WorldToScreen(d.Location) : d.Location;
              DrawStringToHandle(
                  static_cast<int>(drawPos.X),
                  static_cast<int>(drawPos.Y),
                  d.Text.c_str(),
                  d.Color,
                  d.Handle
              );
            }
            // --- Line ---
            else if constexpr (std::is_same_v<T, LineData>) {
              FVector2D p1 = (cmd.common.space == RenderSpace::World)
                                 ? WorldToScreen(d.StartLocation)
                                 : d.StartLocation;
              FVector2D p2 = (cmd.common.space == RenderSpace::World) ? WorldToScreen(d.EndLocation)
                                                                      : d.EndLocation;
              DrawLine(
                  static_cast<int>(p1.X),
                  static_cast<int>(p1.Y),
                  static_cast<int>(p2.X),
                  static_cast<int>(p2.Y),
                  d.Color
              );
            }
            // --- RectGraph ---
            else if constexpr (std::is_same_v<T, RectGraphData>) {
              FVector2D drawPos = (cmd.common.space == RenderSpace::World)
                                      ? WorldToScreen(d.DestLocation)
                                      : d.DestLocation;
              DrawRectGraphF(
                  drawPos.X,
                  drawPos.Y,
                  static_cast<int>(d.SrcLocation.X),
                  static_cast<int>(d.SrcLocation.Y),
                  static_cast<int>(d.SrcSize.X),
                  static_cast<int>(d.SrcSize.Y),
                  d.Handle,
                  TRUE
              );
            }
          },
          cmd.data
      );
    }
  }
  PriorityCommands.clear();
}

void RenderSystem::SetCameraView(MCameraComponent* m) { MainCamera = m; }
MCameraComponent* RenderSystem::GetCamera() { return MainCamera; }
