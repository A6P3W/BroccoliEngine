#pragma once
#include "InputDevice.h"

class MouseDevice : public InputDevice {
 public:
  MouseDevice();
  enum AxisID { Wheel = 0, MouseX, MouseY };

  void Update() override;
  bool GetPressStart(int code) const override;
  bool GetPressing(int code) const override;
  bool GetRelease(int code) const override;
  float GetAxis(int axisID) const override;

 private:
  int Buttons = 0;
  int PrevButtons = 0;
  float WheelDelta = 0.0f;

  int CurrentMouseX = 0;
  int CurrentMouseY = 0;
  int PrevMouseX = 0;
  int PrevMouseY = 0;
};
