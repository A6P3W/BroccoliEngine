#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UMath.h"

enum class RenderType { Graph, Box, Text, Line, RectGraph, Circle };
enum class RenderSpace { World, Screen };

struct GraphData {
  FVector2D Location;
  FRotator Rotation;
  FScale Scale;
  int Handle = 0;
};
struct BoxData {
  FVector2D Location;
  FVector2D WidthHeight;
  FRotator Rotation;
  FColor Color;
  bool Fill;
};
struct TextData {
  FVector2D Location;
  std::string Text;
  FColor Color;
  int Handle = 0;
};
struct LineData {
  FVector2D StartLocation;
  FVector2D EndLocation;
  FColor Color;
};
struct CircleData {
  FVector2D Location;
  float Radius;
  FColor Color;
  bool Fill;
};
struct RectGraphData {
  FVector2D DestLocation;
  FVector2D SrcLocation;
  FVector2D SrcSize;
  int Handle = 0;
};

// Variant でまとめる
using RenderCommandData =
    std::variant<GraphData, BoxData, TextData, LineData, CircleData, RectGraphData>;

struct RenderCommon {
  int priority = 0;
  int alpha = 255;
  RenderSpace space = RenderSpace::World;
};

struct RenderCommand {
  RenderCommon common;
  RenderCommandData data;
};

class MCameraComponent;
struct FRenderContext;
class RenderSystemImpl;

class BROCCOLI_ENGINE_API RenderSystem {
 public:
  RenderSystem();
  ~RenderSystem();
  RenderSystem(const RenderSystem&) = delete;
  RenderSystem& operator=(const RenderSystem&) = delete;
  static RenderSystem& GetInstance();

  void SubmitLine(
      FVector2D Start, FVector2D End, const FColor& Color, RenderSpace Space, int Priority
  );
  void SubmitBox(
      FVector2D Location,
      FVector2D WidthHeight,
      FRotator Rotation,
      const FColor& Color,
      bool Fill,
      RenderSpace Space,
      int Priority
  );
  void SubmitCircle(
      FVector2D Location,
      float Radius,
      const FColor& Color,
      bool Fill,
      RenderSpace Space,
      int Priority
  );
  void SubmitText(
      FVector2D Location,
      const std::string& Text,
      int Handle,
      const FColor& Color,
      RenderSpace Space,
      int Priority
  );
  void SubmitGraph(
      FVector2D Location,
      int Handle,
      FScale Scale,
      FRotator Rotation,
      RenderSpace Space,
      int Priority,
      int Alpha = 255
  );
  void SubmitRectGraph(
      FVector2D Dest,
      FVector2D SrcLoc,
      FVector2D SrcSize,
      int Handle,
      RenderSpace Space,
      int Priority,
      int Alpha = 255
  );

  FVector2D WorldToScreen(const FVector2D& worldPosition) const;
  FVector2D ScreenToWorld(const FVector2D& screenPosition) const;

  void Draw();

  void SetCameraView(MCameraComponent* m);
  MCameraComponent* GetCamera();
  void SetViewCullingEnabled(bool BEnabled);
  bool IsViewCullingEnabled() const;
  void SetViewCullingMargin(float Margin);
  float GetViewCullingMargin() const;
  std::size_t GetLastSubmittedCommandCount() const;
  std::size_t GetLastCulledCommandCount() const;

 private:
  void UpdateDrawStatistics();
  FRenderContext BuildRenderContext() const;
  bool IsCommandVisible(const RenderCommand& Command, const FRenderContext& Context) const;
  void CullCommands(const FRenderContext& Context);
  void SortCommands();
  void DrawCommands(const FRenderContext& Context);
  void DrawCommand(const RenderCommand& Command, const FRenderContext& Context);

  RenderSystemImpl* Impl = nullptr;
};
