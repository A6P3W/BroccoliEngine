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
  return {(std::min)(X1, X2), (std::min)(Y1, Y2), (std::max)(X1, X2), (std::max)(Y1, Y2)};
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

struct FRenderContext {
  FVector2D CameraPosition = FVector2D::ZeroVector();
  float CameraFOV = 1.0f;
  float CameraCos = 1.0f;
  float CameraSin = 0.0f;
  float CameraRotation = 0.0f;
  FScreenBounds ViewBounds;

  FVector2D WorldToScreen(const FVector2D& Position) const {
    const float CenterX = VirtualWidth * 0.5f;
    const float CenterY = VirtualHeight * 0.5f;
    const float LocalX = Position.X - CameraPosition.X;
    const float LocalY = Position.Y - CameraPosition.Y;
    return {
        (LocalX * CameraCos - LocalY * CameraSin) * CameraFOV + CenterX,
        (LocalX * CameraSin + LocalY * CameraCos) * CameraFOV + CenterY
    };
  }

  FVector2D ToScreenPosition(const FVector2D& Position, RenderSpace Space) const {
    return (Space == RenderSpace::World) ? WorldToScreen(Position) : Position;
  }

  float GetSpaceScale(RenderSpace Space) const {
    return (Space == RenderSpace::World) ? CameraFOV : 1.0f;
  }

  float GetSpaceRotation(RenderSpace Space) const {
    return (Space == RenderSpace::World) ? UMath::DegToRad(CameraRotation) : 0.0f;
  }
};

namespace {
struct FGraphDrawParameters {
  FVector2D Position;
  float Scale = 1.0f;
  float Rotation = 0.0f;
};

struct FBoxDrawParameters {
  FVector2D V1;
  FVector2D V2;
  FVector2D V3;
  FVector2D V4;
};

struct FCircleDrawParameters {
  FVector2D Position;
  float Radius = 0.0f;
};

struct FLineDrawParameters {
  FVector2D Start;
  FVector2D End;
};

struct FRectGraphDrawParameters {
  FVector2D Position;
};

FGraphDrawParameters BuildGraphDrawParameters(
    const GraphData& Graph, RenderSpace Space, const FRenderContext& Context
) {
  return {
      Context.ToScreenPosition(Graph.Location, Space),
      Graph.Scale.Scale * Context.GetSpaceScale(Space),
      UMath::DegToRad(Graph.Rotation.Rotation) - Context.GetSpaceRotation(Space)
  };
}

FBoxDrawParameters BuildBoxDrawParameters(
    const BoxData& Box, RenderSpace Space, const FRenderContext& Context
) {
  if (Space == RenderSpace::Screen) {
    const FVector2D V1 = Box.Location;
    const FVector2D V2 = {Box.Location.X + Box.WidthHeight.X, Box.Location.Y};
    const FVector2D V3 = {Box.Location.X + Box.WidthHeight.X, Box.Location.Y + Box.WidthHeight.Y};
    const FVector2D V4 = {Box.Location.X, Box.Location.Y + Box.WidthHeight.Y};
    return {V1, V2, V3, V4};
  }

  const float Rotation = UMath::DegToRad(Box.Rotation.Rotation);
  const float CosA = std::cos(Rotation);
  const float SinA = std::sin(Rotation);
  const auto GetVertex = [&](float LocalX, float LocalY) {
    return Context.WorldToScreen(
        {Box.Location.X + (LocalX * CosA - LocalY * SinA),
         Box.Location.Y + (LocalX * SinA + LocalY * CosA)}
    );
  };

  return {
      GetVertex(0.0f, 0.0f),
      GetVertex(Box.WidthHeight.X, 0.0f),
      GetVertex(Box.WidthHeight.X, Box.WidthHeight.Y),
      GetVertex(0.0f, Box.WidthHeight.Y)
  };
}

FCircleDrawParameters BuildCircleDrawParameters(
    const CircleData& Circle, RenderSpace Space, const FRenderContext& Context
) {
  return {
      Context.ToScreenPosition(Circle.Location, Space), Circle.Radius * Context.GetSpaceScale(Space)
  };
}

FLineDrawParameters BuildLineDrawParameters(
    const LineData& Line, RenderSpace Space, const FRenderContext& Context
) {
  return {
      Context.ToScreenPosition(Line.StartLocation, Space),
      Context.ToScreenPosition(Line.EndLocation, Space)
  };
}

FRectGraphDrawParameters BuildRectGraphDrawParameters(
    const RectGraphData& RectGraph, RenderSpace Space, const FRenderContext& Context
) {
  return {Context.ToScreenPosition(RectGraph.DestLocation, Space)};
}

FScreenBounds MakeBoundsFromBox(const FBoxDrawParameters& Box) {
  return {
      (std::min)({Box.V1.X, Box.V2.X, Box.V3.X, Box.V4.X}),
      (std::min)({Box.V1.Y, Box.V2.Y, Box.V3.Y, Box.V4.Y}),
      (std::max)({Box.V1.X, Box.V2.X, Box.V3.X, Box.V4.X}),
      (std::max)({Box.V1.Y, Box.V2.Y, Box.V3.Y, Box.V4.Y})
  };
}

struct FVisibilityVisitor {
  const RenderCommon& Common;
  const FRenderContext& Context;

  bool operator()(const GraphData& Graph) const {
    const FGraphDrawParameters Parameters = BuildGraphDrawParameters(Graph, Common.space, Context);
    int GraphWidth = 0;
    int GraphHeight = 0;
    if (GetGraphSize(Graph.Handle, &GraphWidth, &GraphHeight) != 0 || GraphWidth <= 0 ||
        GraphHeight <= 0) {
      return true;
    }

    const float HalfWidth = GraphWidth * std::abs(Parameters.Scale) * 0.5f;
    const float HalfHeight = GraphHeight * std::abs(Parameters.Scale) * 0.5f;
    const float AbsCos = std::abs(std::cos(Parameters.Rotation));
    const float AbsSin = std::abs(std::sin(Parameters.Rotation));
    const float ExtentX = AbsCos * HalfWidth + AbsSin * HalfHeight;
    const float ExtentY = AbsSin * HalfWidth + AbsCos * HalfHeight;
    return Intersects(
        MakeScreenBounds(
            Parameters.Position.X - ExtentX,
            Parameters.Position.Y - ExtentY,
            Parameters.Position.X + ExtentX,
            Parameters.Position.Y + ExtentY
        ),
        Context.ViewBounds
    );
  }

  bool operator()(const BoxData& Box) const {
    return Intersects(
        MakeBoundsFromBox(BuildBoxDrawParameters(Box, Common.space, Context)), Context.ViewBounds
    );
  }

  bool operator()(const CircleData& Circle) const {
    const FCircleDrawParameters Parameters =
        BuildCircleDrawParameters(Circle, Common.space, Context);
    const float Radius = std::abs(Parameters.Radius);
    return Intersects(
        MakeScreenBounds(
            Parameters.Position.X - Radius,
            Parameters.Position.Y - Radius,
            Parameters.Position.X + Radius,
            Parameters.Position.Y + Radius
        ),
        Context.ViewBounds
    );
  }

  bool operator()(const TextData& Text) const {
    if (Text.Text.empty()) {
      return false;
    }

    const FVector2D Position = Context.ToScreenPosition(Text.Location, Common.space);
    const int TextWidth = GetDrawStringWidthToHandle(
        Text.Text.c_str(), static_cast<int>(Text.Text.length()), Text.Handle
    );
    const int TextHeight = GetFontSizeToHandle(Text.Handle);
    if (TextWidth < 0 || TextHeight <= 0) {
      return true;
    }

    return Intersects(
        MakeScreenBounds(
            Position.X,
            Position.Y,
            Position.X + static_cast<float>(TextWidth),
            Position.Y + static_cast<float>(TextHeight)
        ),
        Context.ViewBounds
    );
  }

  bool operator()(const LineData& Line) const {
    const FLineDrawParameters Parameters = BuildLineDrawParameters(Line, Common.space, Context);
    return LineIntersects(Parameters.Start, Parameters.End, Context.ViewBounds);
  }

  bool operator()(const RectGraphData& RectGraph) const {
    const FRectGraphDrawParameters Parameters =
        BuildRectGraphDrawParameters(RectGraph, Common.space, Context);
    return Intersects(
        MakeScreenBounds(
            Parameters.Position.X,
            Parameters.Position.Y,
            Parameters.Position.X + RectGraph.SrcSize.X,
            Parameters.Position.Y + RectGraph.SrcSize.Y
        ),
        Context.ViewBounds
    );
  }
};
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
    const FColor& Color,
    bool Fill,
    RenderSpace Space,
    int Priority
) {
  Impl->CommandBuffer.push_back(
      {{Priority, static_cast<int>(Color.A), Space}, CircleData{Location, Radius, Color, Fill}}
  );
}

void RenderSystem::SubmitBox(
    FVector2D Location,
    FVector2D WidthHeight,
    FRotator Rotation,
    const FColor& Color,
    bool Fill,
    RenderSpace Space,
    int Priority
) {
  Impl->CommandBuffer.push_back(
      {{Priority, static_cast<int>(Color.A), Space},
       BoxData{Location, WidthHeight, Rotation, Color, Fill}}
  );
}

void RenderSystem::SubmitText(
    FVector2D Location,
    const std::string& Text,
    int Handle,
    const FColor& Color,
    RenderSpace Space,
    int Priority
) {
  Impl->CommandBuffer.push_back(
      {{Priority, static_cast<int>(Color.A), Space}, TextData{Location, Text, Color, Handle}}
  );
}

void RenderSystem::SubmitLine(
    FVector2D Start, FVector2D End, const FColor& Color, RenderSpace Space, int Priority
) {
  Impl->CommandBuffer.push_back(
      {{Priority, static_cast<int>(Color.A), Space}, LineData{Start, End, Color}}
  );
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

void RenderSystem::UpdateDrawStatistics() {
  Impl->LastSubmittedCommandCount = Impl->CommandBuffer.size();
  Impl->LastCulledCommandCount = 0;
}

FRenderContext RenderSystem::BuildRenderContext() const {
  FVector2D CameraPosition = FVector2D::ZeroVector();
  float CameraRotation = 0.0f;
  float CameraFOV = 1.0f;
  if (Impl->MainCamera) {
    CameraPosition = Impl->MainCamera->GetWorldLocation();
    CameraRotation = Impl->MainCamera->GetWorldRotation().Rotation;
    CameraFOV = Impl->MainCamera->GetFOV();
  }

  const float ViewMargin = Impl->ViewCullingMargin;
  const float CameraRadians = UMath::DegToRad(-CameraRotation);
  return {
      CameraPosition,
      CameraFOV,
      std::cos(CameraRadians),
      std::sin(CameraRadians),
      CameraRotation,
      {-ViewMargin,
       -ViewMargin,
       static_cast<float>(VirtualWidth) + ViewMargin,
       static_cast<float>(VirtualHeight) + ViewMargin}
  };
}

bool RenderSystem::IsCommandVisible(
    const RenderCommand& Command, const FRenderContext& Context
) const {
  return std::visit(FVisibilityVisitor{Command.common, Context}, Command.data);
}

void RenderSystem::CullCommands(const FRenderContext& Context) {
  if (Impl->BViewCullingEnabled) {
    const std::size_t BeforeCullingCount = Impl->CommandBuffer.size();
    Impl->CommandBuffer.erase(
        std::remove_if(
            Impl->CommandBuffer.begin(),
            Impl->CommandBuffer.end(),
            [&](const RenderCommand& Command) { return !IsCommandVisible(Command, Context); }
        ),
        Impl->CommandBuffer.end()
    );
    Impl->LastCulledCommandCount = BeforeCullingCount - Impl->CommandBuffer.size();
  }
}

void RenderSystem::SortCommands() {
  std::stable_sort(
      Impl->CommandBuffer.begin(),
      Impl->CommandBuffer.end(),
      [](const RenderCommand& A, const RenderCommand& B) {
        return A.common.priority < B.common.priority;
      }
  );
}

void RenderSystem::DrawCommands(const FRenderContext& Context) {
  int CurrentBlend = 0;
  int CurrentAlpha = 0;
  GetDrawBlendMode(&CurrentBlend, &CurrentAlpha);

  for (const RenderCommand& Command : Impl->CommandBuffer) {
    if (Command.common.alpha != CurrentAlpha) {
      CurrentAlpha = std::clamp(Command.common.alpha, 0, 255);
      SetDrawBlendMode(DX_BLENDMODE_ALPHA, CurrentAlpha);
    }

    DrawCommand(Command, Context);
  }
}

void RenderSystem::DrawCommand(const RenderCommand& Command, const FRenderContext& Context) {
  std::visit(
      [&](const auto& Data) {
        using T = std::decay_t<decltype(Data)>;

        if constexpr (std::is_same_v<T, GraphData>) {
          const FGraphDrawParameters Parameters =
              BuildGraphDrawParameters(Data, Command.common.space, Context);
          DrawRotaGraphFastF(
              Parameters.Position.X,
              Parameters.Position.Y,
              Parameters.Scale,
              Parameters.Rotation,
              Data.Handle,
              TRUE
          );
        } else if constexpr (std::is_same_v<T, BoxData>) {
          if (Command.common.space == RenderSpace::World) {
            const FBoxDrawParameters Parameters =
                BuildBoxDrawParameters(Data, Command.common.space, Context);
            DrawQuadrangle(
                static_cast<int>(Parameters.V1.X),
                static_cast<int>(Parameters.V1.Y),
                static_cast<int>(Parameters.V2.X),
                static_cast<int>(Parameters.V2.Y),
                static_cast<int>(Parameters.V3.X),
                static_cast<int>(Parameters.V3.Y),
                static_cast<int>(Parameters.V4.X),
                static_cast<int>(Parameters.V4.Y),
                Data.Color.ToRGB(),
                Data.Fill
            );
          } else {
            DrawBox(
                static_cast<int>(Data.Location.X),
                static_cast<int>(Data.Location.Y),
                static_cast<int>(Data.Location.X + Data.WidthHeight.X),
                static_cast<int>(Data.Location.Y + Data.WidthHeight.Y),
                Data.Color.ToRGB(),
                Data.Fill
            );
          }
        } else if constexpr (std::is_same_v<T, CircleData>) {
          const FCircleDrawParameters Parameters =
              BuildCircleDrawParameters(Data, Command.common.space, Context);
          DrawCircle(
              static_cast<int>(Parameters.Position.X),
              static_cast<int>(Parameters.Position.Y),
              static_cast<int>(Parameters.Radius),
              Data.Color.ToRGB(),
              Data.Fill
          );
        } else if constexpr (std::is_same_v<T, TextData>) {
          const FVector2D Position = Context.ToScreenPosition(Data.Location, Command.common.space);
          DrawStringToHandle(
              static_cast<int>(Position.X),
              static_cast<int>(Position.Y),
              Data.Text.c_str(),
              Data.Color.ToRGB(),
              Data.Handle
          );
        } else if constexpr (std::is_same_v<T, LineData>) {
          const FLineDrawParameters Parameters =
              BuildLineDrawParameters(Data, Command.common.space, Context);
          DrawLine(
              static_cast<int>(Parameters.Start.X),
              static_cast<int>(Parameters.Start.Y),
              static_cast<int>(Parameters.End.X),
              static_cast<int>(Parameters.End.Y),
              Data.Color.ToRGB()
          );
        } else if constexpr (std::is_same_v<T, RectGraphData>) {
          const FRectGraphDrawParameters Parameters =
              BuildRectGraphDrawParameters(Data, Command.common.space, Context);
          DrawRectGraphF(
              Parameters.Position.X,
              Parameters.Position.Y,
              static_cast<int>(Data.SrcLocation.X),
              static_cast<int>(Data.SrcLocation.Y),
              static_cast<int>(Data.SrcSize.X),
              static_cast<int>(Data.SrcSize.Y),
              Data.Handle,
              TRUE
          );
        }
      },
      Command.data
  );
}

void RenderSystem::Draw() {
  UpdateDrawStatistics();

  if (Impl->CommandBuffer.empty()) {
    return;
  }

  const FRenderContext Context = BuildRenderContext();
  CullCommands(Context);

  if (Impl->CommandBuffer.empty()) {
    return;
  }

  SortCommands();
  DrawCommands(Context);
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

std::size_t RenderSystem::GetLastCulledCommandCount() const { return Impl->LastCulledCommandCount; }
