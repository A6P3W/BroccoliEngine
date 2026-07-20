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

struct GamepadDevice::Impl {
  int PadInputType = DX_INPUT_PAD1;
  int Buttons = 0;
  int PrevButtons = 0;
  float Axes[6] = {};
};

GamepadDevice::GamepadDevice(int padIndex) : ImplPtr(new Impl()) {
  switch (padIndex) {
    case 1:
      ImplPtr->PadInputType = DX_INPUT_PAD1;
      break;
    case 2:
      ImplPtr->PadInputType = DX_INPUT_PAD2;
      break;
    case 3:
      ImplPtr->PadInputType = DX_INPUT_PAD3;
      break;
    case 4:
      ImplPtr->PadInputType = DX_INPUT_PAD4;
      break;
    default:
      ImplPtr->PadInputType = DX_INPUT_PAD1;
      break;
  }
}

GamepadDevice::~GamepadDevice() { delete ImplPtr; }

void GamepadDevice::Update() {
  ImplPtr->PrevButtons = ImplPtr->Buttons;
  ImplPtr->Buttons = GetJoypadInputState(ImplPtr->PadInputType);

  DxLibXInputStateWrapper xinputState;

  if (GetJoypadXInputState(
          ImplPtr->PadInputType, reinterpret_cast<DxLib::XINPUT_STATE*>(&xinputState)
      ) == 0) {
    ImplPtr->Axes[(int)AxisID::LeftX] = ApplyDeadzone(xinputState.sThumbLX, 0.2f);
    ImplPtr->Axes[(int)AxisID::LeftY] = ApplyDeadzone(xinputState.sThumbLY, 0.2f);
    ImplPtr->Axes[(int)AxisID::RightX] = ApplyDeadzone(xinputState.sThumbRX, 0.2f);
    ImplPtr->Axes[(int)AxisID::RightY] = ApplyDeadzone(xinputState.sThumbRY, 0.2f);
    ImplPtr->Axes[(int)AxisID::LeftTrigger] = (float)xinputState.bLeftTrigger / 255.0f;
    ImplPtr->Axes[(int)AxisID::RightTrigger] = (float)xinputState.bRightTrigger / 255.0f;
  } else {
    int lx = 0, ly = 0;
    GetJoypadAnalogInput(&lx, &ly, ImplPtr->PadInputType);
    ImplPtr->Axes[(int)AxisID::LeftX] = ApplyDeadzone((int)(lx * 32.768f), 0.2f);
    ImplPtr->Axes[(int)AxisID::LeftY] = ApplyDeadzone((int)(ly * 32.768f), 0.2f);
    ImplPtr->Axes[(int)AxisID::RightX] = 0.0f;
    ImplPtr->Axes[(int)AxisID::RightY] = 0.0f;
    ImplPtr->Axes[(int)AxisID::LeftTrigger] = 0.0f;
    ImplPtr->Axes[(int)AxisID::RightTrigger] = 0.0f;
  }
}

bool GamepadDevice::HasInputThisFrame() const {
  if ((~ImplPtr->PrevButtons & ImplPtr->Buttons) != 0) return true;
  for (float Axis : ImplPtr->Axes) {
    if (Axis != 0.0f) return true;
  }
  return false;
}

bool GamepadDevice::GetPressStart(int code) const {
  return !(ImplPtr->PrevButtons & code) && (ImplPtr->Buttons & code);
}

bool GamepadDevice::GetPressing(int code) const { return (ImplPtr->Buttons & code); }

bool GamepadDevice::GetRelease(int code) const {
  return (ImplPtr->PrevButtons & code) && !(ImplPtr->Buttons & code);
}

float GamepadDevice::GetAxis(int axisID) const {
  if (axisID >= 0 && axisID < 6) {
    return ImplPtr->Axes[axisID];
  }
  return 0.0f;
}

float GamepadDevice::ApplyDeadzone(int val, float deadzone) {
  float floatVal = (float)val / 32767.0f;
  if (floatVal > 1.0f) floatVal = 1.0f;
  if (floatVal < -1.0f) floatVal = -1.0f;
  if (std::abs(floatVal) < deadzone) {
    return 0.0f;
  }
  if (floatVal > 0.0f) {
    return (floatVal - deadzone) / (1.0f - deadzone);
  } else {
    return (floatVal + deadzone) / (1.0f - deadzone);
  }
}
