#pragma once

#include <cstddef>
#include <meta>
#include <type_traits>

#include "Reflection.h"

#ifdef __INTELLISENSE__
#define EDITOR_PROPERTY(...)
#else
#define EDITOR_PROPERTY(...) [[= FEditorPropertyAnnotation{__VA_ARGS__}]]
#endif

// Annotation values must be structural; std::optional is not structural in GCC 16.
template <class T>
struct TAnnotationOptional {
  T Value{};
  bool Present = false;

  constexpr TAnnotationOptional() = default;
  constexpr TAnnotationOptional(T InValue) : Value(InValue), Present(true) {}
  constexpr explicit operator bool() const { return Present; }
  constexpr T operator*() const { return Value; }
  constexpr T value_or(T Fallback) const { return Present ? Value : Fallback; }
};

struct FAnnotationNumber {
  double Value = 0.0;
  bool Present = false;

  constexpr FAnnotationNumber() = default;

  template <class T>
    requires(std::is_arithmetic_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>)
  constexpr FAnnotationNumber(T InValue) : Value(static_cast<double>(InValue)), Present(true) {}

  constexpr explicit operator bool() const { return Present; }
  constexpr double operator*() const { return Value; }
};

struct FEditorPropertyAnnotation {
  std::meta::info OnEditorChanged{};
  FAnnotationNumber Min, Max, SliderMin, SliderMax;
  TAnnotationOptional<std::size_t> MaxLength;
  EPathFilter PathFilter = EPathFilter::AnyFile;
  char EnumOptions[64] = {};
};
