#pragma once

#include <cstdint>

#include "Color.h"

enum class EShadingModel3D : uint8_t { Unlit, Lit };

struct FMaterial3DDesc {
  EShadingModel3D ShadingModel = EShadingModel3D::Lit;
  FColor BaseColor = FColor::White;
  int BaseColorTextureHandle = 0;
};
