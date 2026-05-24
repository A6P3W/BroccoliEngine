#include "MouseDevice.h"
#include <DxLib.h>


void MouseDevice::Update()
{
	m_prevButtons = m_buttons;
	m_buttons = GetMouseInput();
	m_wheelDelta = static_cast<float>(GetMouseWheelRotVol());
}

bool MouseDevice::GetPressStart(int code) const
{
	return ((m_prevButtons & code) == 0) && ((m_buttons & code) != 0);
}

bool MouseDevice::GetPressing(int code) const
{
	return (m_buttons & code) != 0;
}

bool MouseDevice::GetRelease(int code) const
{
	return ((m_prevButtons & code) != 0) && ((m_buttons & code) == 0);
}

float MouseDevice::GetAxis(int axisID) const
{
	if (axisID == AxisID::Wheel) return m_wheelDelta;
	return 0.0f;
}
