#pragma once
#include "InputDevice.h"

// アナログ軸の識別ID
enum class AxisID { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger };

class BROCCOLI_ENGINE_API GamepadDevice : public InputDevice {
 public:
  GamepadDevice(int padIndex);
  ~GamepadDevice() override;
  GamepadDevice(const GamepadDevice&) = delete;
  GamepadDevice& operator=(const GamepadDevice&) = delete;

  void Update() override;
  EInputDeviceType GetDeviceType() const override { return EInputDeviceType::Gamepad; }
  bool HasInputThisFrame() const override;
  bool GetPressStart(int code) const override;
  bool GetPressing(int code) const override;
  bool GetRelease(int code) const override;
  float GetAxis(int axisID) const override;

 private:
  float ApplyDeadzone(int val, float deadzone);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
