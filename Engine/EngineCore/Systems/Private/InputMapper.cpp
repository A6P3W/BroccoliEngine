#include "InputMapper.h"
#include "InputDevice.h"
#include "UMath.h"
void InputMapper::AddMapping(const std::string& actionName, InputDevice* device, int code, const std::string& modifierAction, float scale) {
	ButtonBindings[actionName].push_back({ device, code, scale, modifierAction });
}
void InputMapper::AddAxisMapping(const std::string& actionName, InputDevice* device, int axisId, float scale) {
	AxisBindings[actionName].push_back({ device, axisId, scale });
}
void InputMapper::RemoveMapping(const std::string& actionName) {
	ButtonBindings.erase(actionName);
	AxisBindings.erase(actionName);
}
bool InputMapper::GetPressStart(const std::string& actionName) const {
	auto it = ButtonBindings.find(actionName);
	if (it == ButtonBindings.end()) return false;
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
	auto it = ButtonBindings.find(actionName);
	if (it == ButtonBindings.end()) return false;
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
	auto it = ButtonBindings.find(actionName);
	if (it == ButtonBindings.end()) return false;
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
	auto it = AxisBindings.find(actionName);
	if (it != AxisBindings.end())
		for (auto& b : it->second)
			result += b.Device->GetAxis(b.AxisId) * b.Scale;
	// ボタンでの軸エミュレート
	auto bit = ButtonBindings.find(actionName);
	if (bit != ButtonBindings.end())
		for (auto& b : bit->second) {
			if (b.Device->GetPressing(b.Code)) result += b.Scale;
		}
	return result;
}

FVector2D InputMapper::GetAxis2DValue(const std::string& actionNameX, const std::string& actionNameY) const
{
	FVector2D result = FVector2D::ZeroVector;
	result.X = GetAxisValue(actionNameX);
	result.Y = GetAxisValue(actionNameY);
	return result;

}
