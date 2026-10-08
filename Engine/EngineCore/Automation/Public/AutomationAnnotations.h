#pragma once

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>

template <std::size_t Capacity>
struct FAnnotationText {
  std::array<char, Capacity> Data{};
  std::size_t Size = 0;

  consteval FAnnotationText() = default;

  template <std::size_t N>
  consteval FAnnotationText(const char (&Text)[N]) : Size(N - 1) {
    if (Size > Capacity) throw "Control annotation text exceeds its capacity.";
    for (std::size_t Index = 0; Index < Size; ++Index) Data[Index] = Text[Index];
  }

  constexpr std::string_view View() const { return {Data.data(), Size}; }
};

struct FControlMethodAnnotation {
  FAnnotationText<128> Name;
  FAnnotationText<512> Description;
  std::meta::info ResultAdapter{};
};

struct FControlParameterAnnotation {
  std::size_t Index = 0;
  FAnnotationText<128> Name;
  FAnnotationText<512> Description;
};

#ifdef __INTELLISENSE__
#define CONTROL_METHOD(...)
#define CONTROL_PARAMETER(...)
#else
#define CONTROL_METHOD(...) [[= FControlMethodAnnotation{__VA_ARGS__}]]
#define CONTROL_PARAMETER(...) [[= FControlParameterAnnotation{__VA_ARGS__}]]
#endif
