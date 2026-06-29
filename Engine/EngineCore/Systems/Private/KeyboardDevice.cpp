#include "KeyboardDevice.h"

#include <DxLib.h>

void KeyboardDevice::Update() {
  char tmp[256];
  GetHitKeyStateAll(tmp);
  for (int i = 0; i < 256; ++i) {
    PrevKey[i] = Key[i];
    Key[i] = (tmp[i] != 0);
  }
}

bool KeyboardDevice::GetPressStart(int code) const { return !PrevKey[code] && Key[code]; }
bool KeyboardDevice::GetPressing(int code) const { return Key[code]; }
bool KeyboardDevice::GetRelease(int code) const { return PrevKey[code] && !Key[code]; }
