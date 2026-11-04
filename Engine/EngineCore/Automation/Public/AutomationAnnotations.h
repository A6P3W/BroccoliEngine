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

/**
 * @brief メンバ関数をControl APIに公開する。
 *
 * .Name : 公開名
 *
 * .Description : 説明
 *
 * .ResultAdapter : JSON変換関数（任意）
 *
 * @code
 * CONTROL_METHOD(.Name = "open_door", .Description = "Opens the door.")
 * void OpenDoor();
 * @endcode
 */
#define CONTROL_METHOD(...)

/**
 * @brief Control APIに公開する引数情報を指定する。
 *
 * .Index : 引数位置（0から）
 *
 * .Name : JSON上の引数名
 *
 * .Description : 説明（任意）
 *
 * @code
 * CONTROL_PARAMETER(.Index = 0, .Name = "locked")
 * @endcode
 */
#define CONTROL_PARAMETER(...)

#else

#define CONTROL_METHOD(...) [[= FControlMethodAnnotation{__VA_ARGS__}]]
#define CONTROL_PARAMETER(...) [[= FControlParameterAnnotation{__VA_ARGS__}]]

#endif