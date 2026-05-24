#pragma once
#include "InputDevice.h"
#include <string>
#include <vector>
#include <unordered_map>
struct InputAction {
	static constexpr auto MoveX = "MoveX";
	static constexpr auto MoveY = "MoveY";
	static constexpr auto Wheel = "Wheel";
	static constexpr auto Interact = "Interact";
	static constexpr auto Cancel = "Cancel";
};

class InputMapper {
public:
	InputMapper() = default;

	void AddMapping(const std::string& actionName, InputDevice* device, int code, float scale = 1.0f);

	void AddAxisMapping(const std::string& actionName, InputDevice* device, int axisId, float scale = 1.0f);

	void RemoveMapping(const std::string& actionName);

	bool  GetPressStart(const std::string& actionName) const;
	bool  GetPressing(const std::string& actionName) const;
	bool  GetRelease(const std::string& actionName) const;
	float GetAxisValue(const std::string& actionName) const;
private:
	struct FButtonBinding { InputDevice* Device; int Code; float Scale; };
	struct FAxisBinding { InputDevice* Device; int AxisId; float Scale; };
	std::unordered_map<std::string, std::vector<FButtonBinding>> m_buttonBindings;
	std::unordered_map<std::string, std::vector<FAxisBinding>>   m_axisBindings;
};
