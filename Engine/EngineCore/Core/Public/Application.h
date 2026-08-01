#pragma once

#include <memory>

#include "BroccoliEngineAPI.h"

class FAutomationSubsystem;

class BROCCOLI_ENGINE_API Application {
 public:
  Application();
  ~Application();

  bool Run();

  static void SetWindowResolution(int Width, int Height);
  static void SetGameSetupCallback(void (*Callback)());
  static void QuitGame();
  static void* GetImGuiContext();

 private:
  bool Update(float DeltaTime, bool ProcessInput);
  bool Draw(bool CompleteFrame);
  void Shutdown();
  void InitializeAutomation();
  void InitOffscreenBuffer();

  float DeltaTime = 0.0f;
  bool RaylibInitialized = false;
  bool AudioInitialized = false;
  bool ImGuiInitialized = false;
  void* OffscreenBuffer = nullptr;

  std::unique_ptr<FAutomationSubsystem> AutomationSubsystem;
};
