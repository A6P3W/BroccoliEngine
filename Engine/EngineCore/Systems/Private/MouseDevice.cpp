#include "MouseDevice.h"
#include <DxLib.h>


MouseDevice::MouseDevice()
{
	GetMousePoint(&m_mouseX, &m_mouseY);
	m_prevMouseX = m_mouseX;
	m_prevMouseY = m_mouseY;
}

void MouseDevice::Update()
{
	m_prevButtons = m_buttons;
	m_buttons = GetMouseInput();
	m_wheelDelta = static_cast<float>(GetMouseWheelRotVol());
	m_prevMouseX = m_mouseX;
	m_prevMouseY = m_mouseY;
	GetMousePoint(&m_mouseX, &m_mouseY);
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
	if (axisID == AxisID::MouseX) return static_cast<float>(m_mouseX - m_prevMouseX);
	if (axisID == AxisID::MouseY) return static_cast<float>(m_mouseY - m_prevMouseY);
	return 0.0f;
}
