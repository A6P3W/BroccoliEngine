#include "DxLibLap.h"

#include <DxLib.h>

int DxLibLap::MakeScreen(int Width, int Height, bool bUseAlphaChannel) {
  SetDrawScreen(DX_SCREEN_BACK);
  return ::MakeScreen(Width, Height, bUseAlphaChannel);
}

void DxLibLap::ReleaseScreen(int ScreenHandle) { ::DeleteGraph(ScreenHandle); }

int DxLibLap::SetDrawScreen(int ScreenHandle) { return ::SetDrawScreen(ScreenHandle); }

int DxLibLap::GetDrawScreen() { return ::GetDrawScreen(); }

int DxLibLap::ClearDrawScreen() { return ::ClearDrawScreen(); }

int DxLibLap::GetDrawScreenSize(int* Width, int* Height) {
  return ::GetDrawScreenSize(Width, Height);
}
