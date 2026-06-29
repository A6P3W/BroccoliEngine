#pragma once
#include "InputDevice.h"

// アナログ軸の識別ID
enum class AxisID { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger };

class GamepadDevice : public InputDevice {
 public:
  GamepadDevice(int padIndex);

  void Update() override;
  bool GetPressStart(int code) const override;
  bool GetPressing(int code) const override;
  bool GetRelease(int code) const override;
  float GetAxis(int axisID) const override;

 private:
  float ApplyDeadzone(int val, float deadzone);

  int PadInputType;
  int Buttons = 0;
  int PrevButtons = 0;
  float Axes[6] = {};
};
