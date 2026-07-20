#pragma once
#include <memory>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "InputDevice.h"

class KeyboardDevice;
class BROCCOLI_ENGINE_API InputManager {
 private:
  InputManager();
  ~InputManager();

 public:
  static InputManager& GetInstance();
  InputManager(const InputManager&) = delete;
  InputManager& operator=(const InputManager&) = delete;
  void AddDevice(std::unique_ptr<InputDevice> Device);
  std::vector<InputDevice*> GetDevices() const;

  template <class T>
  T* GetDevice() const {
    for (InputDevice* Device : GetDevices()) {
      if (auto* Result = dynamic_cast<T*>(Device)) return Result;
    }
    return nullptr;
  }

  void Update();
  EInputDeviceType GetLastInputDevice() const;
  bool IsLastInputDevice(EInputDeviceType DeviceType) const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
