#pragma once

#include "InputDevice.h"
#include "InputTypes.h"

class BROCCOLI_ENGINE_API MouseDevice : public InputDevice {
 public:
  MouseDevice();
  ~MouseDevice() override;
  MouseDevice(const MouseDevice&) = delete;
  MouseDevice& operator=(const MouseDevice&) = delete;

  enum AxisID { Wheel = 0, MouseX, MouseY };

  void Update() override;
  EInputDeviceType GetDeviceType() const override { return EInputDeviceType::Mouse; }
  bool HasInputThisFrame() const override;
  bool GetPressStart(int Code) const override;
  bool GetPressing(int Code) const override;
  bool GetRelease(int Code) const override;
  float GetAxis(int AxisId) const override;

  bool GetPressStart(EMouseButton Button) const { return GetPressStart(static_cast<int>(Button)); }
  bool GetPressing(EMouseButton Button) const { return GetPressing(static_cast<int>(Button)); }
  bool GetRelease(EMouseButton Button) const { return GetRelease(static_cast<int>(Button)); }

  int GetMouseX() const;
  int GetMouseY() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
