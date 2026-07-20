#pragma once
#include "BroccoliEngineAPI.h"

enum class EInputDeviceType { None, Keyboard, Mouse, Gamepad };

class BROCCOLI_ENGINE_API InputDevice {
 public:
  InputDevice();
  virtual ~InputDevice();
  InputDevice(const InputDevice&) = delete;
  InputDevice& operator=(const InputDevice&) = delete;

  virtual void Update() = 0;
  virtual EInputDeviceType GetDeviceType() const = 0;
  virtual bool HasInputThisFrame() const = 0;
  virtual bool GetPressStart(int code) const = 0;
  virtual bool GetPressing(int code) const = 0;
  virtual bool GetRelease(int code) const = 0;
  virtual float GetAxis(int axisID) const { return 0.0f; }

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
