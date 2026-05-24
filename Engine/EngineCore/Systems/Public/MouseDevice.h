#pragma once
#include "InputDevice.h"

class MouseDevice : public InputDevice {
public:
	enum AxisID {
		Wheel = 0,
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
};
