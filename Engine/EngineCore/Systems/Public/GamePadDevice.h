#pragma once

#include "InputDevice.h"
#include "InputTypes.h"

enum class AxisID { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger };

class BROCCOLI_ENGINE_API GamepadDevice : public InputDevice {
 public:
  explicit GamepadDevice(int PadIndex);
  ~GamepadDevice() override;
  GamepadDevice(const GamepadDevice&) = delete;
  GamepadDevice& operator=(const GamepadDevice&) = delete;

  void Update() override;
  EInputDeviceType GetDeviceType() const override { return EInputDeviceType::Gamepad; }
  bool HasInputThisFrame() const override;
  bool GetPressStart(int Code) const override;
  bool GetPressing(int Code) const override;
  bool GetRelease(int Code) const override;
  float GetAxis(int AxisId) const override;

  bool GetPressStart(EGamepadButton Button) const {
    return GetPressStart(static_cast<int>(Button));
  }
  bool GetPressing(EGamepadButton Button) const { return GetPressing(static_cast<int>(Button)); }
  bool GetRelease(EGamepadButton Button) const { return GetRelease(static_cast<int>(Button)); }

 private:
  static float ApplyDeadzone(float Value, float Deadzone);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
