#include "MouseDevice.h"

#include <DxLib.h>

MouseDevice::MouseDevice() {
  GetMousePoint(&CurrentMouseX, &CurrentMouseY);
  PrevMouseX = CurrentMouseX;
  PrevMouseY = CurrentMouseY;
}

void MouseDevice::Update() {
  PrevButtons = Buttons;
  Buttons = GetMouseInput();
  WheelDelta = static_cast<float>(GetMouseWheelRotVol());
  PrevMouseX = CurrentMouseX;
  PrevMouseY = CurrentMouseY;
  GetMousePoint(&CurrentMouseX, &CurrentMouseY);
}

bool MouseDevice::GetPressStart(int code) const {
  return ((PrevButtons & code) == 0) && ((Buttons & code) != 0);
}

bool MouseDevice::GetPressing(int code) const { return (Buttons & code) != 0; }

bool MouseDevice::GetRelease(int code) const {
  return ((PrevButtons & code) != 0) && ((Buttons & code) == 0);
}

float MouseDevice::GetAxis(int axisID) const {
  if (axisID == AxisID::Wheel) return WheelDelta;
  if (axisID == AxisID::MouseX) return static_cast<float>(CurrentMouseX - PrevMouseX);
  if (axisID == AxisID::MouseY) return static_cast<float>(CurrentMouseY - PrevMouseY);
  return 0.0f;
}
