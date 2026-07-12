#pragma once
#include "InputDevice.h"

class BROCCOLI_ENGINE_API KeyboardDevice : public InputDevice {
 public:
  KeyboardDevice();
  ~KeyboardDevice() override;
  KeyboardDevice(const KeyboardDevice&) = delete;
  KeyboardDevice& operator=(const KeyboardDevice&) = delete;

  void Update() override;
  bool GetPressStart(int code) const override;
  bool GetPressing(int code) const override;
  bool GetRelease(int code) const override;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
