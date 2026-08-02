#include "MouseDevice.h"

#include <array>
#include <cstddef>

#include "BroccoliRaylib.h"
#include "EngineDefine.h"

namespace {
constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(EMouseButton::Count);

int ToRaylibMouseButton(EMouseButton Button) {
  static constexpr std::array<int, MouseButtonCount> ButtonMap = {
      MOUSE_BUTTON_LEFT,
      MOUSE_BUTTON_RIGHT,
      MOUSE_BUTTON_MIDDLE,
  };
  const std::size_t Index = static_cast<std::size_t>(Button);
  return Index < ButtonMap.size() ? ButtonMap[Index] : MOUSE_BUTTON_LEFT;
}

bool IsValidButtonCode(int Code) {
  return Code >= 0 && static_cast<std::size_t>(Code) < MouseButtonCount;
}

void GetVirtualMousePoint(int& OutX, int& OutY) {
  const Vector2 MousePosition = GetMousePosition();
  const int ScreenWidth = GetScreenWidth();
  const int ScreenHeight = GetScreenHeight();
  if (ScreenWidth <= 0 || ScreenHeight <= 0) {
    OutX = static_cast<int>(MousePosition.x);
    OutY = static_cast<int>(MousePosition.y);
    return;
  }

  const float ScaleX = static_cast<float>(ScreenWidth) / VirtualWidth;
  const float ScaleY = static_cast<float>(ScreenHeight) / VirtualHeight;
  const float Scale = (std::min)(ScaleX, ScaleY);
  if (Scale <= 0.0f) {
    OutX = 0;
    OutY = 0;
    return;
  }

  const int DrawWidth = static_cast<int>(VirtualWidth * Scale);
  const int DrawHeight = static_cast<int>(VirtualHeight * Scale);
  const int DrawX = (ScreenWidth - DrawWidth) / 2;
  const int DrawY = (ScreenHeight - DrawHeight) / 2;
  OutX = static_cast<int>((MousePosition.x - DrawX) / Scale);
  OutY = static_cast<int>((MousePosition.y - DrawY) / Scale);
}
}  // namespace

struct MouseDevice::Impl {
  std::array<bool, MouseButtonCount> Buttons{};
  std::array<bool, MouseButtonCount> PreviousButtons{};
  float WheelDelta = 0.0f;
  int CurrentMouseX = 0;
  int CurrentMouseY = 0;
  int PreviousMouseX = 0;
  int PreviousMouseY = 0;
};

MouseDevice::MouseDevice() : ImplPtr(new Impl()) {
  GetVirtualMousePoint(ImplPtr->CurrentMouseX, ImplPtr->CurrentMouseY);
  ImplPtr->PreviousMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PreviousMouseY = ImplPtr->CurrentMouseY;
}

MouseDevice::~MouseDevice() { delete ImplPtr; }

void MouseDevice::Update() {
  ImplPtr->PreviousButtons = ImplPtr->Buttons;
  for (std::size_t Index = 0; Index < MouseButtonCount; ++Index) {
    ImplPtr->Buttons[Index] =
        IsMouseButtonDown(ToRaylibMouseButton(static_cast<EMouseButton>(Index)));
  }
  ImplPtr->WheelDelta = GetMouseWheelMove();
  ImplPtr->PreviousMouseX = ImplPtr->CurrentMouseX;
  ImplPtr->PreviousMouseY = ImplPtr->CurrentMouseY;
  GetVirtualMousePoint(ImplPtr->CurrentMouseX, ImplPtr->CurrentMouseY);
}

bool MouseDevice::HasInputThisFrame() const {
  for (std::size_t Index = 0; Index < MouseButtonCount; ++Index) {
    if (!ImplPtr->PreviousButtons[Index] && ImplPtr->Buttons[Index]) return true;
  }
  const bool Moved = ImplPtr->CurrentMouseX != ImplPtr->PreviousMouseX ||
                     ImplPtr->CurrentMouseY != ImplPtr->PreviousMouseY;
  return ImplPtr->WheelDelta != 0.0f || Moved;
}

bool MouseDevice::GetPressStart(int Code) const {
  if (!IsValidButtonCode(Code)) return false;
  return !ImplPtr->PreviousButtons[Code] && ImplPtr->Buttons[Code];
}

bool MouseDevice::GetPressing(int Code) const {
  return IsValidButtonCode(Code) && ImplPtr->Buttons[Code];
}

bool MouseDevice::GetRelease(int Code) const {
  if (!IsValidButtonCode(Code)) return false;
  return ImplPtr->PreviousButtons[Code] && !ImplPtr->Buttons[Code];
}

float MouseDevice::GetAxis(int AxisId) const {
  if (AxisId == AxisID::Wheel) return ImplPtr->WheelDelta;
  if (AxisId == AxisID::MouseX) {
    return static_cast<float>(ImplPtr->CurrentMouseX - ImplPtr->PreviousMouseX);
  }
  if (AxisId == AxisID::MouseY) {
    return static_cast<float>(ImplPtr->CurrentMouseY - ImplPtr->PreviousMouseY);
  }
  return 0.0f;
}

int MouseDevice::GetMouseX() const { return ImplPtr->CurrentMouseX; }

int MouseDevice::GetMouseY() const { return ImplPtr->CurrentMouseY; }
