#pragma once

#include "InputDevice.h"
#include "InputTypes.h"

class BROCCOLI_ENGINE_API KeyboardDevice : public InputDevice {
 public:
  KeyboardDevice();
  ~KeyboardDevice() override;
  KeyboardDevice(const KeyboardDevice&) = delete;
  KeyboardDevice& operator=(const KeyboardDevice&) = delete;

  void Update() override;
  EInputDeviceType GetDeviceType() const override { return EInputDeviceType::Keyboard; }
  bool HasInputThisFrame() const override;
  bool GetPressStart(int Code) const override;
  bool GetPressing(int Code) const override;
  bool GetRelease(int Code) const override;

  bool GetPressStart(EKey Key) const { return GetPressStart(static_cast<int>(Key)); }
  bool GetPressing(EKey Key) const { return GetPressing(static_cast<int>(Key)); }
  bool GetRelease(EKey Key) const { return GetRelease(static_cast<int>(Key)); }

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
