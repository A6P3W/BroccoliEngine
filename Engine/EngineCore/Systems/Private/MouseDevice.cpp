#include "MouseDevice.h"

#include <DxLib.h>

#include "EngineDefine.h"

namespace {
void GetVirtualMousePoint(int& OutX, int& OutY) {
  int Mx = 0, My = 0;
  GetMousePoint(&Mx, &My);

  int ScreenW = 0, ScreenH = 0;
  GetDrawScreenSize(&ScreenW, &ScreenH);

  if (ScreenW == 0 || ScreenH == 0) {
    OutX = Mx;
    OutY = My;
    return;
  }

  // Application::Draw() と同じスケーリング計算
  float ScaleX = static_cast<float>(ScreenW) / VirtualWidth;
  float ScaleY = static_cast<float>(ScreenH) / VirtualHeight;
  float Scale = (ScaleX < ScaleY) ? ScaleX : ScaleY;

  int DrawW = static_cast<int>(VirtualWidth * Scale);
  int DrawH = static_cast<int>(VirtualHeight * Scale);
  int DrawX = (ScreenW - DrawW) / 2;
  int DrawY = (ScreenH - DrawH) / 2;

  // ウィンドウの生座標から仮想解像度 (VirtualWidth x VirtualHeight) の座標へ逆変換
  OutX = static_cast<int>((Mx - DrawX) / Scale);
  OutY = static_cast<int>((My - DrawY) / Scale);
}
}  // namespace

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
  GetVirtualMousePoint(ImplPtr->CurrentMouseX, ImplPtr->CurrentMouseY);
  ImplPtr->PrevMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PrevMouseY = ImplPtr->CurrentMouseY;
}

MouseDevice::~MouseDevice() { delete ImplPtr; }

void MouseDevice::Update() {
  ImplPtr->PrevButtons = ImplPtr->Buttons;
  ImplPtr->Buttons = GetMouseInput();
  ImplPtr->WheelDelta = static_cast<float>(GetMouseWheelRotVol());
  ImplPtr->PrevMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PrevMouseY = ImplPtr->CurrentMouseY;

  // 仮想座標を取得して更新
  GetVirtualMousePoint(ImplPtr->CurrentMouseX, ImplPtr->CurrentMouseY);
}

bool MouseDevice::HasInputThisFrame() const {
  const bool bButtonPressed = (~ImplPtr->PrevButtons & ImplPtr->Buttons) != 0;
  const bool bMoved = ImplPtr->CurrentMouseX != ImplPtr->PrevMouseX ||
                      ImplPtr->CurrentMouseY != ImplPtr->PrevMouseY;
  return bButtonPressed || ImplPtr->WheelDelta != 0.0f || bMoved;
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

int MouseDevice::GetMouseX() const { return ImplPtr->CurrentMouseX; }

int MouseDevice::GetMouseY() const { return ImplPtr->CurrentMouseY; }
