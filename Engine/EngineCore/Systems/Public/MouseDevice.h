#pragma once
#include "InputDevice.h"

class MouseDevice : public InputDevice {
public:
	MouseDevice();
	enum AxisID {
		Wheel = 0,
		MouseX,
		MouseY
	};

	void Update() override;
	bool GetPressStart(int code) const override;
	bool GetPressing(int code) const override;
	bool GetRelease(int code) const override;
	float GetAxis(int axisID) const override;

private:
	int m_buttons = 0;
	int m_prevButtons = 0;
	float m_wheelDelta = 0.0f;

	int m_mouseX = 0;
	int m_mouseY = 0;
	int m_prevMouseX = 0;
	int m_prevMouseY = 0;
};
