#include "InputMapper.h"

#include "GamePadDevice.h"
#include "InputDevice.h"
#include "KeyboardDevice.h"
#include "MouseDevice.h"
#include "UMath.h"

struct InputMapper::Impl {
  std::unordered_map<std::string, std::vector<FButtonBinding>> ButtonBindings;
  std::unordered_map<std::string, std::vector<FAxisBinding>> AxisBindings;
};

InputMapper::InputMapper() : ImplPtr(new Impl()) {}

InputMapper::~InputMapper() { delete ImplPtr; }

void InputMapper::AddMapping(
    const std::string& ActionName,
    KeyboardDevice* Device,
    EKey Key,
    const std::string& ModifierAction,
    float Scale
) {
  AddButtonMapping(ActionName, Device, static_cast<int>(Key), ModifierAction, Scale);
}

void InputMapper::AddMapping(
    const std::string& ActionName,
    MouseDevice* Device,
    EMouseButton Button,
    const std::string& ModifierAction,
    float Scale
) {
  AddButtonMapping(ActionName, Device, static_cast<int>(Button), ModifierAction, Scale);
}

void InputMapper::AddMapping(
    const std::string& ActionName,
    GamepadDevice* Device,
    EGamepadButton Button,
    const std::string& ModifierAction,
    float Scale
) {
  AddButtonMapping(ActionName, Device, static_cast<int>(Button), ModifierAction, Scale);
}

void InputMapper::AddButtonMapping(
    const std::string& ActionName,
    InputDevice* Device,
    int Code,
    const std::string& ModifierAction,
    float Scale
) {
  if (Device == nullptr) {
    return;
  }
  ImplPtr->ButtonBindings[ActionName].push_back({Device, Code, Scale, ModifierAction});
}

void InputMapper::AddAxisMapping(
    const std::string& ActionName, InputDevice* Device, int AxisId, float Scale
) {
  if (Device == nullptr) {
    return;
  }
  ImplPtr->AxisBindings[ActionName].push_back({Device, AxisId, Scale});
}

void InputMapper::RemoveMapping(const std::string& ActionName) {
  ImplPtr->ButtonBindings.erase(ActionName);
  ImplPtr->AxisBindings.erase(ActionName);
}

bool InputMapper::GetPressStart(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetPressStart(Binding.Code)) return true;
  }
  return false;
}

bool InputMapper::GetPressing(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetPressing(Binding.Code)) return true;
  }
  return false;
}

bool InputMapper::GetRelease(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetRelease(Binding.Code)) return true;
  }
  return false;
}

float InputMapper::GetAxisValue(const std::string& ActionName) const {
  float Result = 0.0f;
  const auto AxisIt = ImplPtr->AxisBindings.find(ActionName);
  if (AxisIt != ImplPtr->AxisBindings.end()) {
    for (const auto& Binding : AxisIt->second) {
      Result += Binding.Device->GetAxis(Binding.AxisId) * Binding.Scale;
    }
  }

  const auto ButtonIt = ImplPtr->ButtonBindings.find(ActionName);
  if (ButtonIt != ImplPtr->ButtonBindings.end()) {
    for (const auto& Binding : ButtonIt->second) {
      if (Binding.Device->GetPressing(Binding.Code)) Result += Binding.Scale;
    }
  }
  return Result;
}

FVector2D InputMapper::GetAxis2DValue(
    const std::string& ActionNameX, const std::string& ActionNameY
) const {
  FVector2D Result = FVector2D::ZeroVector();
  Result.X = GetAxisValue(ActionNameX);
  Result.Y = GetAxisValue(ActionNameY);
  return Result;
}

EInputDeviceType InputMapper::GetPressStartDevice(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return EInputDeviceType::None;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetPressStart(Binding.Code)) {
      return Binding.Device->GetDeviceType();
    }
  }
  return EInputDeviceType::None;
}

EInputDeviceType InputMapper::GetPressingDevice(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return EInputDeviceType::None;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetPressing(Binding.Code)) {
      return Binding.Device->GetDeviceType();
    }
  }
  return EInputDeviceType::None;
}

EInputDeviceType InputMapper::GetReleaseDevice(const std::string& ActionName) const {
  const auto It = ImplPtr->ButtonBindings.find(ActionName);
  if (It == ImplPtr->ButtonBindings.end()) return EInputDeviceType::None;
  for (const auto& Binding : It->second) {
    bool ModifierMet = true;
    if (!Binding.ModifierAction.empty()) {
      ModifierMet = GetPressing(Binding.ModifierAction);
    }
    if (ModifierMet && Binding.Device->GetRelease(Binding.Code)) {
      return Binding.Device->GetDeviceType();
    }
  }
  return EInputDeviceType::None;
}

EInputDeviceType InputMapper::GetAxisValueDevice(const std::string& ActionName) const {
  const auto AxisIt = ImplPtr->AxisBindings.find(ActionName);
  if (AxisIt != ImplPtr->AxisBindings.end()) {
    for (const auto& Binding : AxisIt->second) {
      if (Binding.Device->GetAxis(Binding.AxisId) * Binding.Scale != 0.0f) {
        return Binding.Device->GetDeviceType();
      }
    }
  }

  const auto ButtonIt = ImplPtr->ButtonBindings.find(ActionName);
  if (ButtonIt != ImplPtr->ButtonBindings.end()) {
    for (const auto& Binding : ButtonIt->second) {
      if (Binding.Device->GetPressing(Binding.Code) && Binding.Scale != 0.0f) {
        return Binding.Device->GetDeviceType();
      }
    }
  }
  return EInputDeviceType::None;
}

EInputDeviceType InputMapper::GetAxis2DValueDevice(
    const std::string& ActionNameX, const std::string& ActionNameY
) const {
  const EInputDeviceType DeviceType = GetAxisValueDevice(ActionNameX);
  if (DeviceType != EInputDeviceType::None) return DeviceType;
  return GetAxisValueDevice(ActionNameY);
}
