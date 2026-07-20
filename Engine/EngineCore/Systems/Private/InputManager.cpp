#include "InputManager.h"

struct InputManager::Impl {
  std::vector<std::unique_ptr<InputDevice>> Devices;
  EInputDeviceType LastInputDevice = EInputDeviceType::None;
};

InputManager& InputManager::GetInstance() {
  static InputManager Instance;
  return Instance;
}

InputManager::InputManager() : ImplPtr(new Impl()) {}

InputManager::~InputManager() { delete ImplPtr; }

void InputManager::AddDevice(std::unique_ptr<InputDevice> Device) {
  ImplPtr->Devices.push_back(std::move(Device));
}

std::vector<InputDevice*> InputManager::GetDevices() const {
  std::vector<InputDevice*> Result;
  Result.reserve(ImplPtr->Devices.size());
  for (const auto& Device : ImplPtr->Devices) {
    Result.push_back(Device.get());
  }
  return Result;
}

void InputManager::Update() {
  for (auto& Device : ImplPtr->Devices) {
    Device->Update();
    if (Device->HasInputThisFrame()) {
      ImplPtr->LastInputDevice = Device->GetDeviceType();
    }
  }
}

EInputDeviceType InputManager::GetLastInputDevice() const { return ImplPtr->LastInputDevice; }

bool InputManager::IsLastInputDevice(EInputDeviceType DeviceType) const {
  return ImplPtr->LastInputDevice == DeviceType;
}
