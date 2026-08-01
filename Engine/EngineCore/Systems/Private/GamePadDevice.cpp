#include "GamePadDevice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "BroccoliRaylib.h"

namespace {
constexpr std::size_t GamepadButtonCount = static_cast<std::size_t>(EGamepadButton::Count);
constexpr std::size_t GamepadAxisCount = 6;

int ToRaylibGamepadButton(EGamepadButton Button) {
  static constexpr std::array<int, GamepadButtonCount> ButtonMap = {
      GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
      GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
      GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
      GAMEPAD_BUTTON_RIGHT_FACE_UP,
      GAMEPAD_BUTTON_MIDDLE_RIGHT,
      GAMEPAD_BUTTON_MIDDLE_LEFT,
      GAMEPAD_BUTTON_LEFT_FACE_UP,
      GAMEPAD_BUTTON_LEFT_FACE_DOWN,
      GAMEPAD_BUTTON_LEFT_FACE_LEFT,
      GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
      GAMEPAD_BUTTON_LEFT_TRIGGER_1,
      GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
      GAMEPAD_BUTTON_LEFT_THUMB,
      GAMEPAD_BUTTON_RIGHT_THUMB,
  };
  const std::size_t Index = static_cast<std::size_t>(Button);
  return Index < ButtonMap.size() ? ButtonMap[Index] : GAMEPAD_BUTTON_UNKNOWN;
}

bool IsValidButtonCode(int Code) {
  return Code >= 0 && static_cast<std::size_t>(Code) < GamepadButtonCount;
}

float NormalizeTrigger(float Value) { return std::clamp((Value + 1.0f) * 0.5f, 0.0f, 1.0f); }
}  // namespace

struct GamepadDevice::Impl {
  int GamepadIndex = 0;
  std::array<bool, GamepadButtonCount> Buttons{};
  std::array<bool, GamepadButtonCount> PreviousButtons{};
  std::array<float, GamepadAxisCount> Axes{};
};

GamepadDevice::GamepadDevice(int PadIndex) : ImplPtr(new Impl()) {
  ImplPtr->GamepadIndex = (std::max)(0, PadIndex - 1);
}

GamepadDevice::~GamepadDevice() { delete ImplPtr; }

void GamepadDevice::Update() {
  ImplPtr->PreviousButtons = ImplPtr->Buttons;
  if (!IsGamepadAvailable(ImplPtr->GamepadIndex)) {
    ImplPtr->Buttons.fill(false);
    ImplPtr->Axes.fill(0.0f);
    return;
  }

  for (std::size_t Index = 0; Index < GamepadButtonCount; ++Index) {
    ImplPtr->Buttons[Index] = IsGamepadButtonDown(
        ImplPtr->GamepadIndex, ToRaylibGamepadButton(static_cast<EGamepadButton>(Index))
    );
  }

  ImplPtr->Axes[static_cast<int>(AxisID::LeftX)] =
      ApplyDeadzone(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_LEFT_X), 0.2f);
  ImplPtr->Axes[static_cast<int>(AxisID::LeftY)] =
      ApplyDeadzone(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_LEFT_Y), 0.2f);
  ImplPtr->Axes[static_cast<int>(AxisID::RightX)] =
      ApplyDeadzone(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_RIGHT_X), 0.2f);
  ImplPtr->Axes[static_cast<int>(AxisID::RightY)] =
      ApplyDeadzone(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_RIGHT_Y), 0.2f);
  ImplPtr->Axes[static_cast<int>(AxisID::LeftTrigger)] =
      NormalizeTrigger(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_LEFT_TRIGGER));
  ImplPtr->Axes[static_cast<int>(AxisID::RightTrigger)] =
      NormalizeTrigger(GetGamepadAxisMovement(ImplPtr->GamepadIndex, GAMEPAD_AXIS_RIGHT_TRIGGER));
}

bool GamepadDevice::HasInputThisFrame() const {
  for (std::size_t Index = 0; Index < GamepadButtonCount; ++Index) {
    if (!ImplPtr->PreviousButtons[Index] && ImplPtr->Buttons[Index]) return true;
  }
  for (float Axis : ImplPtr->Axes) {
    if (Axis != 0.0f) return true;
  }
  return false;
}

bool GamepadDevice::GetPressStart(int Code) const {
  if (!IsValidButtonCode(Code)) return false;
  return !ImplPtr->PreviousButtons[Code] && ImplPtr->Buttons[Code];
}

bool GamepadDevice::GetPressing(int Code) const {
  return IsValidButtonCode(Code) && ImplPtr->Buttons[Code];
}

bool GamepadDevice::GetRelease(int Code) const {
  if (!IsValidButtonCode(Code)) return false;
  return ImplPtr->PreviousButtons[Code] && !ImplPtr->Buttons[Code];
}

float GamepadDevice::GetAxis(int AxisId) const {
  if (AxisId >= 0 && static_cast<std::size_t>(AxisId) < ImplPtr->Axes.size()) {
    return ImplPtr->Axes[AxisId];
  }
  return 0.0f;
}

float GamepadDevice::ApplyDeadzone(float Value, float Deadzone) {
  Value = std::clamp(Value, -1.0f, 1.0f);
  if (std::abs(Value) < Deadzone) return 0.0f;
  if (Value > 0.0f) return (Value - Deadzone) / (1.0f - Deadzone);
  return (Value + Deadzone) / (1.0f - Deadzone);
}
