#pragma once

#include <cstdint>

#include "UMath.h"

enum class ESpriteBillboardMode : uint8_t {
  None,
  FaceCamera,
  FaceCameraPlane,
  FaceCameraYAxis,
};

enum class RenderType { Graph, Box, Text, Line, RectGraph, Circle };
enum class RenderSpace { World, Screen };
enum class ERenderLayer3D : uint8_t { World, Overlay };
enum class EGridPlane { XZ, XY, YZ };

struct FScreenRenderArea {
  FVector2D Position = FVector2D::ZeroVector();
  FVector2D Size = FVector2D::ZeroVector();
  float Scale = 1.0f;
};
