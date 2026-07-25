#pragma once
#include <memory>
#include <vector>

#include "BroccoliEngineAPI.h"
class AGameModeBase;
class FAutomationApiController;
class FAutomationCommandQueue;
class FAutomationHttpServer;
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
  void ShutdownAutomation();

  float DeltaTime = 0.0f;
  bool bPosed = false;
  bool bDxLibInitialized = false;
  bool bImGuiInitialized = false;

  void InitOffscreenBuffer();
  int OffscreenBuffer = -1;

  std::unique_ptr<FAutomationCommandQueue> AutomationCommandQueue;
  std::unique_ptr<FAutomationApiController> AutomationApiController;
  std::unique_ptr<FAutomationHttpServer> AutomationHttpServer;
};
