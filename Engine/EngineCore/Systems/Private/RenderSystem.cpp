#include "RenderSystem.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "CameraComponent.h"
#include "DxLib.h"
#include "EngineDefine.h"
#include "Log.h"

namespace {
struct FScreenBounds {
  float MinX = 0.0f;
  float MinY = 0.0f;
  float MaxX = 0.0f;
  float MaxY = 0.0f;
};

FScreenBounds MakeScreenBounds(float X1, float Y1, float X2, float Y2) {
  return {
      (std::min)(X1, X2),
      (std::min)(Y1, Y2),
      (std::max)(X1, X2),
      (std::max)(Y1, Y2)
  };
}

bool Intersects(const FScreenBounds& A, const FScreenBounds& B) {
  return !(A.MaxX < B.MinX || A.MinX > B.MaxX || A.MaxY < B.MinY || A.MinY > B.MaxY);
}

bool ClipLine(float P, float Q, float& MinT, float& MaxT) {
  if (std::abs(P) < 1e-6f) {
    return Q >= 0.0f;
  }

  const float Ratio = Q / P;
  if (P < 0.0f) {
    if (Ratio > MaxT) {
      return false;
    }
    MinT = (std::max)(MinT, Ratio);
  } else {
    if (Ratio < MinT) {
      return false;
    }
    MaxT = (std::min)(MaxT, Ratio);
  }
  return true;
}

bool LineIntersects(const FVector2D& Start, const FVector2D& End, const FScreenBounds& Bounds) {
  const float DeltaX = End.X - Start.X;
  const float DeltaY = End.Y - Start.Y;
  float MinT = 0.0f;
  float MaxT = 1.0f;
  return ClipLine(-DeltaX, Start.X - Bounds.MinX, MinT, MaxT) &&
         ClipLine(DeltaX, Bounds.MaxX - Start.X, MinT, MaxT) &&
         ClipLine(-DeltaY, Start.Y - Bounds.MinY, MinT, MaxT) &&
         ClipLine(DeltaY, Bounds.MaxY - Start.Y, MinT, MaxT);
}
}  // namespace

class RenderSystemImpl {
 public:
  std::vector<RenderCommand> CommandBuffer;
  MCameraComponent* MainCamera = nullptr;
  bool BViewCullingEnabled = true;
  float ViewCullingMargin = 8.0f;
  std::size_t LastSubmittedCommandCount = 0;
  std::size_t LastCulledCommandCount = 0;
};

RenderSystem::RenderSystem() : Impl(new RenderSystemImpl()) {}

RenderSystem::~RenderSystem() { delete Impl; }

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
  Impl->CommandBuffer.push_back(
      {{Priority, Alpha, Space}, GraphData{Location, Rotation, Scale, Handle}}
  );
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
  Impl->CommandBuffer.push_back(
      {{Priority, Alpha, Space}, CircleData{Location, Radius, Color, Fill}}
  );
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
  Impl->CommandBuffer.push_back(
      {{Priority, Alpha, Space}, BoxData{Location, WidthHeight, Rotation, Color, Fill}}
  );
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
  Impl->CommandBuffer.push_back(
      {{Priority, Alpha, Space}, TextData{Location, Text, Color, Handle}}
  );
}

void RenderSystem::SubmitLine(
    FVector2D Start, FVector2D End, int Color, RenderSpace Space, int Priority, int Alpha
) {
  Impl->CommandBuffer.push_back({{Priority, Alpha, Space}, LineData{Start, End, Color}});
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
  Impl->CommandBuffer.push_back(
      {{Priority, Alpha, Space}, RectGraphData{Dest, SrcLoc, SrcSize, Handle}}
  );
}

FVector2D RenderSystem::WorldToScreen(const FVector2D& worldPos) const {
  FVector2D camPos = FVector2D::ZeroVector();
  float camRot = 0.0f;
  float camFOV = 1.0f;
  if (Impl->MainCamera) {
    camPos = Impl->MainCamera->GetWorldLocation();
    camRot = Impl->MainCamera->GetWorldRotation().Rotation;
    camFOV = Impl->MainCamera->GetFOV();
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
  FVector2D camPos = FVector2D::ZeroVector();
  float camRot = 0.0f;
  float camFOV = 1.0f;
  if (Impl->MainCamera) {
    camPos = Impl->MainCamera->GetWorldLocation();
    camRot = Impl->MainCamera->GetWorldRotation().Rotation;
    camFOV = Impl->MainCamera->GetFOV();
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
  Impl->LastSubmittedCommandCount = Impl->CommandBuffer.size();
  Impl->LastCulledCommandCount = 0;

  if (Impl->CommandBuffer.empty()) {
    return;
  }

  FVector2D camPos = FVector2D::ZeroVector();
  float camRot = 0.0f;
  float camFOV = 1.0f;
  if (Impl->MainCamera) {
    camPos = Impl->MainCamera->GetWorldLocation();
    camRot = Impl->MainCamera->GetWorldRotation().Rotation;
    camFOV = Impl->MainCamera->GetFOV();
  }

  const float centerX = VirtualWidth * 0.5f;
  const float centerY = VirtualHeight * 0.5f;
  const float camRad = UMath::DegToRad(-camRot);
  const float camCos = std::cos(camRad);
  const float camSin = std::sin(camRad);
  const float drawCamRot = UMath::DegToRad(camRot);

  auto FastWorldToScreen = [&](const FVector2D& worldPos) -> FVector2D {
    const float localX = worldPos.X - camPos.X;
    const float localY = worldPos.Y - camPos.Y;
    return {
        (localX * camCos - localY * camSin) * camFOV + centerX,
        (localX * camSin + localY * camCos) * camFOV + centerY
    };
  };

  const float ViewMargin = Impl->ViewCullingMargin;
  const FScreenBounds ViewBounds = {
      -ViewMargin,
      -ViewMargin,
      static_cast<float>(VirtualWidth) + ViewMargin,
      static_cast<float>(VirtualHeight) + ViewMargin
  };

  auto IsCommandVisible = [&](const RenderCommand& Command) -> bool {
    return std::visit(
        [&](auto&& Data) -> bool {
          using T = std::decay_t<decltype(Data)>;

          if constexpr (std::is_same_v<T, GraphData>) {
            const FVector2D DrawPos = (Command.common.space == RenderSpace::World)
                                          ? FastWorldToScreen(Data.Location)
                                          : Data.Location;
            const float DrawScale =
                Data.Scale.Scale *
                (Command.common.space == RenderSpace::World ? camFOV : 1.0f);
            const float DrawRotation =
                UMath::DegToRad(Data.Rotation.Rotation) -
                (Command.common.space == RenderSpace::World ? drawCamRot : 0.0f);
            int GraphWidth = 0;
            int GraphHeight = 0;
            if (GetGraphSize(Data.Handle, &GraphWidth, &GraphHeight) != 0 || GraphWidth <= 0 ||
                GraphHeight <= 0) {
              return true;
            }

            const float HalfWidth = GraphWidth * std::abs(DrawScale) * 0.5f;
            const float HalfHeight = GraphHeight * std::abs(DrawScale) * 0.5f;
            const float AbsCos = std::abs(std::cos(DrawRotation));
            const float AbsSin = std::abs(std::sin(DrawRotation));
            const float ExtentX = AbsCos * HalfWidth + AbsSin * HalfHeight;
            const float ExtentY = AbsSin * HalfWidth + AbsCos * HalfHeight;
            return Intersects(
                MakeScreenBounds(
                    DrawPos.X - ExtentX,
                    DrawPos.Y - ExtentY,
                    DrawPos.X + ExtentX,
                    DrawPos.Y + ExtentY
                ),
                ViewBounds
            );
          } else if constexpr (std::is_same_v<T, BoxData>) {
            if (Command.common.space == RenderSpace::World) {
              const float Rotation = UMath::DegToRad(Data.Rotation.Rotation);
              const float CosA = std::cos(Rotation);
              const float SinA = std::sin(Rotation);
              auto GetVertex = [&](float LocalX, float LocalY) {
                return FastWorldToScreen(
                    {
                        Data.Location.X + (LocalX * CosA - LocalY * SinA),
                        Data.Location.Y + (LocalX * SinA + LocalY * CosA)
                    }
                );
              };
              const FVector2D V1 = GetVertex(0.0f, 0.0f);
              const FVector2D V2 = GetVertex(Data.WidthHeight.X, 0.0f);
              const FVector2D V3 = GetVertex(Data.WidthHeight.X, Data.WidthHeight.Y);
              const FVector2D V4 = GetVertex(0.0f, Data.WidthHeight.Y);
              const FScreenBounds Bounds = {
                  (std::min)({V1.X, V2.X, V3.X, V4.X}),
                  (std::min)({V1.Y, V2.Y, V3.Y, V4.Y}),
                  (std::max)({V1.X, V2.X, V3.X, V4.X}),
                  (std::max)({V1.Y, V2.Y, V3.Y, V4.Y})
              };
              return Intersects(Bounds, ViewBounds);
            }

            return Intersects(
                MakeScreenBounds(
                    Data.Location.X,
                    Data.Location.Y,
                    Data.Location.X + Data.WidthHeight.X,
                    Data.Location.Y + Data.WidthHeight.Y
                ),
                ViewBounds
            );
          } else if constexpr (std::is_same_v<T, CircleData>) {
            const FVector2D DrawPos = (Command.common.space == RenderSpace::World)
                                          ? FastWorldToScreen(Data.Location)
                                          : Data.Location;
            const float DrawRadius = std::abs(
                Data.Radius *
                (Command.common.space == RenderSpace::World ? camFOV : 1.0f)
            );
            return Intersects(
                MakeScreenBounds(
                    DrawPos.X - DrawRadius,
                    DrawPos.Y - DrawRadius,
                    DrawPos.X + DrawRadius,
                    DrawPos.Y + DrawRadius
                ),
                ViewBounds
            );
          } else if constexpr (std::is_same_v<T, TextData>) {
            if (Data.Text.empty()) {
              return false;
            }

            const FVector2D DrawPos = (Command.common.space == RenderSpace::World)
                                          ? FastWorldToScreen(Data.Location)
                                          : Data.Location;
            const int TextWidth = GetDrawStringWidthToHandle(
                Data.Text.c_str(), static_cast<int>(Data.Text.length()), Data.Handle
            );
            const int TextHeight = GetFontSizeToHandle(Data.Handle);
            if (TextWidth < 0 || TextHeight <= 0) {
              return true;
            }

            return Intersects(
                MakeScreenBounds(
                    DrawPos.X,
                    DrawPos.Y,
                    DrawPos.X + static_cast<float>(TextWidth),
                    DrawPos.Y + static_cast<float>(TextHeight)
                ),
                ViewBounds
            );
          } else if constexpr (std::is_same_v<T, LineData>) {
            const FVector2D Start = (Command.common.space == RenderSpace::World)
                                        ? FastWorldToScreen(Data.StartLocation)
                                        : Data.StartLocation;
            const FVector2D End = (Command.common.space == RenderSpace::World)
                                      ? FastWorldToScreen(Data.EndLocation)
                                      : Data.EndLocation;
            return LineIntersects(Start, End, ViewBounds);
          } else if constexpr (std::is_same_v<T, RectGraphData>) {
            const FVector2D DrawPos = (Command.common.space == RenderSpace::World)
                                          ? FastWorldToScreen(Data.DestLocation)
                                          : Data.DestLocation;
            return Intersects(
                MakeScreenBounds(
                    DrawPos.X,
                    DrawPos.Y,
                    DrawPos.X + Data.SrcSize.X,
                    DrawPos.Y + Data.SrcSize.Y
                ),
                ViewBounds
            );
          }

          return true;
        },
        Command.data
    );
  };

  if (Impl->BViewCullingEnabled) {
    const std::size_t BeforeCullingCount = Impl->CommandBuffer.size();
    Impl->CommandBuffer.erase(
        std::remove_if(
            Impl->CommandBuffer.begin(),
            Impl->CommandBuffer.end(),
            [&](const RenderCommand& Command) { return !IsCommandVisible(Command); }
        ),
        Impl->CommandBuffer.end()
    );
    Impl->LastCulledCommandCount = BeforeCullingCount - Impl->CommandBuffer.size();
  }

  if (Impl->CommandBuffer.empty()) {
    return;
  }

  std::stable_sort(
      Impl->CommandBuffer.begin(),
      Impl->CommandBuffer.end(),
      [](const RenderCommand& a, const RenderCommand& b) {
        return a.common.priority < b.common.priority;
      }
  );

  int curBlend, curAlpha;
  GetDrawBlendMode(&curBlend, &curAlpha);

  // 3. 描画処理実行
  for (const auto& cmd : Impl->CommandBuffer) {
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
            FVector2D drawPos = (cmd.common.space == RenderSpace::World)
                                    ? FastWorldToScreen(d.Location)
                                    : d.Location;
            float drawScale =
                d.Scale.Scale * (cmd.common.space == RenderSpace::World ? camFOV : 1.0f);
            float drawRot = UMath::DegToRad(d.Rotation.Rotation) -
                            (cmd.common.space == RenderSpace::World ? drawCamRot : 0.0f);
            DrawRotaGraphFastF(drawPos.X, drawPos.Y, drawScale, drawRot, d.Handle, TRUE);
          }
          // --- Box ---
          else if constexpr (std::is_same_v<T, BoxData>) {
            if (cmd.common.space == RenderSpace::World) {
              float rad = UMath::DegToRad(d.Rotation.Rotation);
              float cosA = std::cos(rad), sinA = std::sin(rad);
              auto GetV = [&](float lx, float ly) {
                return FastWorldToScreen(
                    {d.Location.X + (lx * cosA - ly * sinA), d.Location.Y + (lx * sinA + ly * cosA)}
                );
              };
              FVector2D v1 = GetV(0, 0), v2 = GetV(d.WidthHeight.X, 0),
                        v3 = GetV(d.WidthHeight.X, d.WidthHeight.Y), v4 = GetV(0, d.WidthHeight.Y);
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
            FVector2D drawPos = (cmd.common.space == RenderSpace::World)
                                    ? FastWorldToScreen(d.Location)
                                    : d.Location;
            float drawRadius = d.Radius * (cmd.common.space == RenderSpace::World ? camFOV : 1.0f);
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
            FVector2D drawPos = (cmd.common.space == RenderSpace::World)
                                    ? FastWorldToScreen(d.Location)
                                    : d.Location;
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
                               ? FastWorldToScreen(d.StartLocation)
                               : d.StartLocation;
            FVector2D p2 = (cmd.common.space == RenderSpace::World)
                               ? FastWorldToScreen(d.EndLocation)
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
                                    ? FastWorldToScreen(d.DestLocation)
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

  Impl->CommandBuffer.clear();
}

void RenderSystem::SetCameraView(MCameraComponent* m) { Impl->MainCamera = m; }
MCameraComponent* RenderSystem::GetCamera() { return Impl->MainCamera; }

void RenderSystem::SetViewCullingEnabled(bool BEnabled) { Impl->BViewCullingEnabled = BEnabled; }

bool RenderSystem::IsViewCullingEnabled() const { return Impl->BViewCullingEnabled; }

void RenderSystem::SetViewCullingMargin(float Margin) {
  Impl->ViewCullingMargin = (std::max)(0.0f, Margin);
}

float RenderSystem::GetViewCullingMargin() const { return Impl->ViewCullingMargin; }

std::size_t RenderSystem::GetLastSubmittedCommandCount() const {
  return Impl->LastSubmittedCommandCount;
}

std::size_t RenderSystem::GetLastCulledCommandCount() const {
  return Impl->LastCulledCommandCount;
}
