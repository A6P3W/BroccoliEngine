#pragma once

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API DxLibLap {
 public:
  static int MakeScreen(int Width, int Height, bool bUseAlphaChannel = false);
  static void ReleaseScreen(int ScreenHandle);
  static int SetDrawScreen(int ScreenHandle);
  static int GetDrawScreen();
  static int ClearDrawScreen();
  static int GetDrawScreenSize(int* Width, int* Height);
};
