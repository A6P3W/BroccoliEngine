#include "KeyboardDevice.h"

#include <DxLib.h>

struct KeyboardDevice::Impl {
  bool Key[256] = {};
  bool PrevKey[256] = {};
};

KeyboardDevice::KeyboardDevice() : ImplPtr(new Impl()) {}

KeyboardDevice::~KeyboardDevice() {
  delete ImplPtr;
}

void KeyboardDevice::Update() {
  char tmp[256];
  GetHitKeyStateAll(tmp);
  for (int i = 0; i < 256; ++i) {
    ImplPtr->PrevKey[i] = ImplPtr->Key[i];
    ImplPtr->Key[i] = (tmp[i] != 0);
  }
}

bool KeyboardDevice::GetPressStart(int code) const {
  return !ImplPtr->PrevKey[code] && ImplPtr->Key[code];
}

bool KeyboardDevice::GetPressing(int code) const { return ImplPtr->Key[code]; }

bool KeyboardDevice::GetRelease(int code) const {
  return ImplPtr->PrevKey[code] && !ImplPtr->Key[code];
}
