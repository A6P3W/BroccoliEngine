#pragma once
#include "BroccoliEngineAPI.h"
#include <memory>
#include <vector>
class AGameModeBase;
class BROCCOLI_ENGINE_API Application {
 public:
  Application();
  ~Application();
  bool Run();

  static void SetWindowResolution(int width, int height);
   static void SetGameSetupCallback(void (*Callback)());
   static void QuitGame();
   static void* GetImGuiContext();

 private:
  bool Update(float DeltaTime);
  bool Draw();
  void Shutdown();

  float DeltaTime = 0.0f;
  bool bPosed = false;
  bool bDxLibInitialized = false;
  bool bImGuiInitialized = false;

  void InitOffscreenBuffer();
  int OffscreenBuffer = -1;
};
