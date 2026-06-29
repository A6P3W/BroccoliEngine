#pragma once
#include <memory>
#include <vector>

#include "InputDevice.h"

class KeyboardDevice;
class InputManager {
 private:
  InputManager();
  ~InputManager();

 public:
  static InputManager& GetInstance() {
    static InputManager instance;
    return instance;
  }
  void AddDevice(std::unique_ptr<InputDevice> device);

  template <class T>
  T* GetDevice() const {
    for (const auto& d : Devices) {
      if (auto* p = dynamic_cast<T*>(d.get())) return p;
    }
    return nullptr;
  }

  void Update();

 private:
  std::vector<std::unique_ptr<InputDevice>> Devices;
};
