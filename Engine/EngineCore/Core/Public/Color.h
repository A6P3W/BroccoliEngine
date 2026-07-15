#pragma once

#include <cstdint>

#include "BroccoliEngineAPI.h"

struct BROCCOLI_ENGINE_API FColor {
  uint8_t R = 0;
  uint8_t G = 0;
  uint8_t B = 0;
  uint8_t A = 255;

  constexpr FColor() = default;
  constexpr FColor(uint8_t InR, uint8_t InG, uint8_t InB, uint8_t InA = 255)
      : R(InR), G(InG), B(InB), A(InA) {}

  constexpr int ToRGB() const {
    return (static_cast<int>(R) << 16) | (static_cast<int>(G) << 8) | static_cast<int>(B);
  }

  static const FColor Transparent;
  static const FColor Black;
  static const FColor White;
  static const FColor Gray;
  static const FColor LightGray;
  static const FColor DarkGray;
  static const FColor Red;
  static const FColor Green;
  static const FColor Blue;
  static const FColor Yellow;
  static const FColor Orange;
  static const FColor Cyan;
  static const FColor Magenta;
};
