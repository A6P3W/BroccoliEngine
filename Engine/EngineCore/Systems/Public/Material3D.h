#pragma once

#include <cstdint>

enum class EShadingModel3D : uint8_t { Unlit = 0, Lit = 1 };

struct FMaterial3DDesc {
  EShadingModel3D ShadingModel = EShadingModel3D::Lit;
};
