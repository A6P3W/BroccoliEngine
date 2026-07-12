#include "MouseDevice.h"

#include <DxLib.h>

struct MouseDevice::Impl {
  int Buttons = 0;
  int PrevButtons = 0;
  float WheelDelta = 0.0f;

  int CurrentMouseX = 0;
  int CurrentMouseY = 0;
  int PrevMouseX = 0;
  int PrevMouseY = 0;
};

MouseDevice::MouseDevice() : ImplPtr(new Impl()) {
  GetMousePoint(&ImplPtr->CurrentMouseX, &ImplPtr->CurrentMouseY);
  ImplPtr->PrevMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PrevMouseY = ImplPtr->CurrentMouseY;
}

MouseDevice::~MouseDevice() {
  delete ImplPtr;
}

void MouseDevice::Update() {
  ImplPtr->PrevButtons = ImplPtr->Buttons;
  ImplPtr->Buttons = GetMouseInput();
  ImplPtr->WheelDelta = static_cast<float>(GetMouseWheelRotVol());
  ImplPtr->PrevMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PrevMouseY = ImplPtr->CurrentMouseY;
  GetMousePoint(&ImplPtr->CurrentMouseX, &ImplPtr->CurrentMouseY);
}

bool MouseDevice::GetPressStart(int code) const {
  return ((ImplPtr->PrevButtons & code) == 0) && ((ImplPtr->Buttons & code) != 0);
}

bool MouseDevice::GetPressing(int code) const { return (ImplPtr->Buttons & code) != 0; }

bool MouseDevice::GetRelease(int code) const {
  return ((ImplPtr->PrevButtons & code) != 0) && ((ImplPtr->Buttons & code) == 0);
}

float MouseDevice::GetAxis(int axisID) const {
  if (axisID == AxisID::Wheel) return ImplPtr->WheelDelta;
  if (axisID == AxisID::MouseX) {
    return static_cast<float>(ImplPtr->CurrentMouseX - ImplPtr->PrevMouseX);
  }
  if (axisID == AxisID::MouseY) {
    return static_cast<float>(ImplPtr->CurrentMouseY - ImplPtr->PrevMouseY);
  }
  return 0.0f;
}
