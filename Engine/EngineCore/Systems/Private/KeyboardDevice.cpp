#include "KeyboardDevice.h"

#include <array>
#include <cstddef>

#include "BroccoliRaylib.h"

namespace {
constexpr std::size_t KeyCount = static_cast<std::size_t>(EKey::Count);

int ToRaylibKey(EKey Key) {
  static constexpr std::array<int, KeyCount> KeyMap = {
      KEY_A,
      KEY_B,
      KEY_C,
      KEY_D,
      KEY_E,
      KEY_F,
      KEY_G,
      KEY_H,
      KEY_I,
      KEY_J,
      KEY_K,
      KEY_L,
      KEY_M,
      KEY_N,
      KEY_O,
      KEY_P,
      KEY_Q,
      KEY_R,
      KEY_S,
      KEY_T,
      KEY_U,
      KEY_V,
      KEY_W,
      KEY_X,
      KEY_Y,
      KEY_Z,
      KEY_ZERO,
      KEY_ONE,
      KEY_TWO,
      KEY_THREE,
      KEY_FOUR,
      KEY_FIVE,
      KEY_SIX,
      KEY_SEVEN,
      KEY_EIGHT,
      KEY_NINE,
      KEY_UP,
      KEY_DOWN,
      KEY_LEFT,
      KEY_RIGHT,
      KEY_SPACE,
      KEY_ENTER,
      KEY_ESCAPE,
      KEY_BACKSPACE,
      KEY_DELETE,
      KEY_LEFT_SHIFT,
      KEY_RIGHT_SHIFT,
      KEY_LEFT_CONTROL,
      KEY_RIGHT_CONTROL,
  };
  const std::size_t Index = static_cast<std::size_t>(Key);
  return Index < KeyMap.size() ? KeyMap[Index] : KEY_NULL;
}

bool IsValidKeyCode(int Code) { return Code >= 0 && static_cast<std::size_t>(Code) < KeyCount; }
}  // namespace

struct KeyboardDevice::Impl {
  std::array<bool, KeyCount> Keys{};
  std::array<bool, KeyCount> PreviousKeys{};
};

KeyboardDevice::KeyboardDevice() : ImplPtr(new Impl()) {}

KeyboardDevice::~KeyboardDevice() { delete ImplPtr; }

void KeyboardDevice::Update() {
  ImplPtr->PreviousKeys = ImplPtr->Keys;
  for (std::size_t Index = 0; Index < KeyCount; ++Index) {
    ImplPtr->Keys[Index] = IsKeyDown(ToRaylibKey(static_cast<EKey>(Index)));
  }
}

bool KeyboardDevice::HasInputThisFrame() const {
  for (std::size_t Index = 0; Index < KeyCount; ++Index) {
    if (!ImplPtr->PreviousKeys[Index] && ImplPtr->Keys[Index]) return true;
  }
  return false;
}

bool KeyboardDevice::GetPressStart(int Code) const {
  if (!IsValidKeyCode(Code)) return false;
  return !ImplPtr->PreviousKeys[Code] && ImplPtr->Keys[Code];
}

bool KeyboardDevice::GetPressing(int Code) const {
  return IsValidKeyCode(Code) && ImplPtr->Keys[Code];
}

bool KeyboardDevice::GetRelease(int Code) const {
  if (!IsValidKeyCode(Code)) return false;
  return ImplPtr->PreviousKeys[Code] && !ImplPtr->Keys[Code];
}
