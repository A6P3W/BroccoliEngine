#include "GamepadDevice.h"
#include <DxLib.h>
#include <cmath>

struct DxLibXInputStateWrapper {
	unsigned char Buttons[16];
	unsigned char bLeftTrigger;
	unsigned char bRightTrigger;
	short sThumbLX;
	short sThumbLY;
	short sThumbRX;
	short sThumbRY;
};

GamepadDevice::GamepadDevice(int padIndex)
{
	switch (padIndex) {
	case 1:  PadInputType = DX_INPUT_PAD1;  break;
	case 2:  PadInputType = DX_INPUT_PAD2;  break;
	case 3:  PadInputType = DX_INPUT_PAD3;  break;
	case 4:  PadInputType = DX_INPUT_PAD4;  break;
	default: PadInputType = DX_INPUT_PAD1;  break;
	}
}

void GamepadDevice::Update()
{
	PrevButtons = Buttons;
	Buttons = GetJoypadInputState(PadInputType);

	DxLibXInputStateWrapper xinputState;

	if (GetJoypadXInputState(PadInputType, reinterpret_cast<DxLib::XINPUT_STATE*>(&xinputState)) == 0) {
		Axes[(int)AxisID::LeftX] = ApplyDeadzone(xinputState.sThumbLX, 0.2f);
		Axes[(int)AxisID::LeftY] = ApplyDeadzone(xinputState.sThumbLY, 0.2f);
		Axes[(int)AxisID::RightX] = ApplyDeadzone(xinputState.sThumbRX, 0.2f);
		Axes[(int)AxisID::RightY] = ApplyDeadzone(xinputState.sThumbRY, 0.2f);
		Axes[(int)AxisID::LeftTrigger] = (float)xinputState.bLeftTrigger / 255.0f;
		Axes[(int)AxisID::RightTrigger] = (float)xinputState.bRightTrigger / 255.0f;
	}
	else {
		int lx = 0, ly = 0;
		GetJoypadAnalogInput(&lx, &ly, PadInputType);
		Axes[(int)AxisID::LeftX] = ApplyDeadzone((int)(lx * 32.768f), 0.2f);
		Axes[(int)AxisID::LeftY] = ApplyDeadzone((int)(ly * 32.768f), 0.2f);
		Axes[(int)AxisID::RightX] = 0.0f;
		Axes[(int)AxisID::RightY] = 0.0f;
		Axes[(int)AxisID::LeftTrigger] = 0.0f;
		Axes[(int)AxisID::RightTrigger] = 0.0f;
	}
}

bool GamepadDevice::GetPressStart(int code) const
{
	return !(PrevButtons & code) && (Buttons & code);
}

bool GamepadDevice::GetPressing(int code) const
{
	return (Buttons & code);
}

bool GamepadDevice::GetRelease(int code) const
{
	return (PrevButtons & code) && !(Buttons & code);
}

float GamepadDevice::GetAxis(int axisID) const
{
	if (axisID >= 0 && axisID < 6) {
		return Axes[axisID];
	}
	return 0.0f;
}

float GamepadDevice::ApplyDeadzone(int val, float deadzone)
{
	float floatVal = (float)val / 32767.0f;
	if (floatVal > 1.0f)  floatVal = 1.0f;
	if (floatVal < -1.0f) floatVal = -1.0f;
	if (std::abs(floatVal) < deadzone) {
		return 0.0f;
	}
	if (floatVal > 0.0f) {
		return (floatVal - deadzone) / (1.0f - deadzone);
	}
	else {
		return (floatVal + deadzone) / (1.0f - deadzone);
	}
}
