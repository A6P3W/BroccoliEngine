#include "InputMapper.h"

#include "InputDevice.h"
#include "UMath.h"

struct InputMapper::Impl {
  std::unordered_map<std::string, std::vector<FButtonBinding>> ButtonBindings;
  std::unordered_map<std::string, std::vector<FAxisBinding>> AxisBindings;
};

InputMapper::InputMapper() : ImplPtr(new Impl()) {}

InputMapper::~InputMapper() {
  delete ImplPtr;
}
void InputMapper::AddMapping(
    const std::string& actionName,
    InputDevice* device,
    int code,
    const std::string& modifierAction,
    float scale
) {
  ImplPtr->ButtonBindings[actionName].push_back({device, code, scale, modifierAction});
}
void InputMapper::AddAxisMapping(
    const std::string& actionName, InputDevice* device, int axisId, float scale
) {
  ImplPtr->AxisBindings[actionName].push_back({device, axisId, scale});
}
void InputMapper::RemoveMapping(const std::string& actionName) {
  ImplPtr->ButtonBindings.erase(actionName);
  ImplPtr->AxisBindings.erase(actionName);
}
bool InputMapper::GetPressStart(const std::string& actionName) const {
  auto it = ImplPtr->ButtonBindings.find(actionName);
  if (it == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& b : it->second) {
    bool bModifierMet = true;
    if (!b.ModifierAction.empty()) {
      bModifierMet = GetPressing(b.ModifierAction);
    }
    if (bModifierMet && b.Device->GetPressStart(b.Code)) return true;
  }
  return false;
}
bool InputMapper::GetPressing(const std::string& actionName) const {
  auto it = ImplPtr->ButtonBindings.find(actionName);
  if (it == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& b : it->second) {
    bool bModifierMet = true;
    if (!b.ModifierAction.empty()) {
      bModifierMet = GetPressing(b.ModifierAction);
    }
    if (bModifierMet && b.Device->GetPressing(b.Code)) return true;
  }
  return false;
}
bool InputMapper::GetRelease(const std::string& actionName) const {
  auto it = ImplPtr->ButtonBindings.find(actionName);
  if (it == ImplPtr->ButtonBindings.end()) return false;
  for (const auto& b : it->second) {
    bool bModifierMet = true;
    if (!b.ModifierAction.empty()) {
      bModifierMet = GetPressing(b.ModifierAction);
    }
    if (bModifierMet && b.Device->GetRelease(b.Code)) return true;
  }
  return false;
}
float InputMapper::GetAxisValue(const std::string& actionName) const {
  float result = 0.0f;
  auto it = ImplPtr->AxisBindings.find(actionName);
  if (it != ImplPtr->AxisBindings.end())
    for (auto& b : it->second) result += b.Device->GetAxis(b.AxisId) * b.Scale;
  // ボタンでの軸エミュレート
  auto bit = ImplPtr->ButtonBindings.find(actionName);
  if (bit != ImplPtr->ButtonBindings.end())
    for (auto& b : bit->second) {
      if (b.Device->GetPressing(b.Code)) result += b.Scale;
    }
  return result;
}

FVector2D InputMapper::GetAxis2DValue(
    const std::string& actionNameX, const std::string& actionNameY
) const {
  FVector2D result = FVector2D::ZeroVector();
  result.X = GetAxisValue(actionNameX);
  result.Y = GetAxisValue(actionNameY);
  return result;
}
