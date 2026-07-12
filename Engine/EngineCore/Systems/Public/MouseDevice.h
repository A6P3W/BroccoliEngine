#pragma once
#include "InputDevice.h"

class BROCCOLI_ENGINE_API MouseDevice : public InputDevice {
 public:
  MouseDevice();
  ~MouseDevice() override;
  MouseDevice(const MouseDevice&) = delete;
  MouseDevice& operator=(const MouseDevice&) = delete;

  enum AxisID { Wheel = 0, MouseX, MouseY };

  void Update() override;
  bool GetPressStart(int code) const override;
  bool GetPressing(int code) const override;
  bool GetRelease(int code) const override;
  float GetAxis(int axisID) const override;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
