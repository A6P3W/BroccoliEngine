#pragma once

#include <memory>

#include "BroccoliEngineAPI.h"
#include "EngineUpdateControl.h"

class FAutomationSubsystem;

class BROCCOLI_ENGINE_API Application {
 public:
  Application();
  ~Application();

  bool Run();

  static void SetWindowResolution(int Width, int Height);
  static void SetGameSetupCallback(void (*Callback)());
  static void QuitGame();
  static FEngineUpdateControl* GetUpdateControl();
  static void* GetImGuiContext();

 private:
  bool EngineUpdate(float DeltaTime, bool ProcessInput);
  void ControlUpdate();
  bool ProcessSceneChanges();
  void Tick(float DeltaTime, bool ProcessInput, bool CompleteFrame);
  bool Draw(bool CompleteFrame);
  void Shutdown();
  void InitializeAutomation();
  bool InitOffscreenBuffer(int Width, int Height);

  FEngineUpdateControl UpdateControl;
  bool TickInProgress = false;
  float DeltaTime = 0.0f;
  bool RaylibInitialized = false;
  bool AudioInitialized = false;
  bool ImGuiInitialized = false;
  void* OffscreenBuffer = nullptr;

  std::unique_ptr<FAutomationSubsystem> AutomationSubsystem;
};
