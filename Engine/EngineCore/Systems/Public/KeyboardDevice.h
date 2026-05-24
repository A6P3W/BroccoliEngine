#pragma once
#include "InputDevice.h"

class KeyboardDevice :public InputDevice {
public:
	void Update() override;
	bool GetPressStart(int code) const override;
	bool GetPressing(int code)   const override;
	bool GetRelease(int code)    const override;

private:
	bool m_key[256] = {};
	bool m_prevKey[256] = {};
};
