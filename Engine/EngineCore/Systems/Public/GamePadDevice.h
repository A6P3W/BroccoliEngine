#pragma once
#include "InputDevice.h"

// アナログ軸の識別ID
enum class AxisID {
	LeftX,
	LeftY,
	RightX,
	RightY,
	LeftTrigger,
	RightTrigger
};

class GamepadDevice : public InputDevice {
public:
	GamepadDevice(int padIndex);

	void Update() override;
	bool GetPressStart(int code) const override;
	bool GetPressing(int code) const override;
	bool GetRelease(int code) const override;
	float GetAxis(int axisID) const override;

private:
	float ApplyDeadzone(int val, float deadzone);

	int m_padInputType;
	int m_buttons = 0;
	int m_prevButtons = 0;
	float m_axes[6] = {};
};
