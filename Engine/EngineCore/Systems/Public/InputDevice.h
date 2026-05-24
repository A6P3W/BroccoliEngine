#pragma once


class InputDevice {
public:
	virtual ~InputDevice() = default;
	virtual void Update() = 0;
	virtual bool GetPressStart(int code) const = 0;
	virtual bool GetPressing(int code) const = 0;
	virtual bool GetRelease(int code) const = 0;
	virtual float GetAxis(int axisID)const { return 0.0f; }

};
