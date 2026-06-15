#include "GamePadDevice.h"
#include <DxLib.h>
#include <cmath>

GamePadDevice::GamePadDevice(int padIndex)
{
	switch (padIndex) {
	case 1:  m_padInputType = DX_INPUT_PAD1;  break;
	case 2:  m_padInputType = DX_INPUT_PAD2;  break;
	case 3:  m_padInputType = DX_INPUT_PAD3;  break;
	case 4:  m_padInputType = DX_INPUT_PAD4;  break;
	default: m_padInputType = DX_INPUT_PAD1;  break;
	}
}

void GamePadDevice::Update()
{
	m_prevButtons = m_buttons;
	m_buttons = GetJoypadInputState(m_padInputType);

	XINPUT_STATE xinputState;
	if (GetJoypadXInputState(m_padInputType, &xinputState) == 0) {
		// XInput対応ゲームパッドの場合
		m_axes[(int)AxisID::LeftX]  = ApplyDeadzone(xinputState.ThumbLX, 0.2f);
		m_axes[(int)AxisID::LeftY]  = ApplyDeadzone(xinputState.ThumbLY, 0.2f);
		m_axes[(int)AxisID::RightX] = ApplyDeadzone(xinputState.ThumbRX, 0.2f);
		m_axes[(int)AxisID::RightY] = ApplyDeadzone(xinputState.ThumbRY, 0.2f);

		m_axes[(int)AxisID::LeftTrigger]  = (float)xinputState.LeftTrigger / 255.0f;
		m_axes[(int)AxisID::RightTrigger] = (float)xinputState.RightTrigger / 255.0f;
	} else {
		// DirectInput対応ゲームパッドの場合
		int lx = 0, ly = 0;
		GetJoypadAnalogInput(&lx, &ly, m_padInputType);
		
		// DirectInputの生値 -1000 ～ 1000 を、XInput相当の -32768 ～ 32767 の範囲にスケーリング
		m_axes[(int)AxisID::LeftX] = ApplyDeadzone((int)(lx * 32.768f), 0.2f);
		m_axes[(int)AxisID::LeftY] = ApplyDeadzone((int)(ly * 32.768f), 0.2f);
		m_axes[(int)AxisID::RightX] = 0.0f;
		m_axes[(int)AxisID::RightY] = 0.0f;
		m_axes[(int)AxisID::LeftTrigger] = 0.0f;
		m_axes[(int)AxisID::RightTrigger] = 0.0f;
	}
}

bool GamePadDevice::GetPressStart(int code) const
{
	return !(m_prevButtons & code) && (m_buttons & code);
}

bool GamePadDevice::GetPressing(int code) const
{
	return (m_buttons & code);
}

bool GamePadDevice::GetRelease(int code) const
{
	return (m_prevButtons & code) && !(m_buttons & code);
}

float GamePadDevice::GetAxis(int axisID) const
{
	if (axisID >= 0 && axisID < 6) {
		return m_axes[axisID];
	}
	return 0.0f;
}

float GamePadDevice::ApplyDeadzone(int val, float deadzone)
{
	// スティックの最大値を 32767.0f として -1.0f ～ 1.0f に正規化
	float floatVal = (float)val / 32767.0f;
	if (floatVal > 1.0f)  floatVal = 1.0f;
	if (floatVal < -1.0f) floatVal = -1.0f;

	if (std::abs(floatVal) < deadzone) {
		return 0.0f;
	}

	// デッドゾーンの外側の値を補間してスムーズなアナログ入力にする
	if (floatVal > 0.0f) {
		return (floatVal - deadzone) / (1.0f - deadzone);
	} else {
		return (floatVal + deadzone) / (1.0f - deadzone);
	}
}
