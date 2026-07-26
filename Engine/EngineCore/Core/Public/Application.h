#pragma once
#include <memory>
#include <vector>

#include "BroccoliEngineAPI.h"
class AGameModeBase;
class FAutomationSubsystem;
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
  void InitializeAutomation();

  float DeltaTime = 0.0f;
  bool bDxLibInitialized = false;
  bool bImGuiInitialized = false;

  void InitOffscreenBuffer();
  int OffscreenBuffer = -1;

  std::unique_ptr<FAutomationSubsystem> AutomationSubsystem;
};
