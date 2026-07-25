#include "Application.h"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl/imgui_impl_dx11.h>
#include <imgui_impl/imgui_impl_win32.h>
#include <shellapi.h>

#include <filesystem>

#include "ActorManager.h"
#include "AutomationApiController.h"
#include "AutomationCommandQueue.h"
#include "AutomationHttpServer.h"
#include "AutomationTypes.h"
#include "CollisionSystem.h"
#include "DebugOverlay.h"
#include "DxLib.h"
#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "EOSTitleStorageManager.h"
#include "EditorMode.h"
#include "EngineDefine.h"
#include "GameModeBase.h"
#include "GamePadDevice.h"
#include "HttpManager.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "KeyboardDevice.h"
#include "Log.h"
#include "MouseDevice.h"
#include "NetworkManager.h"
#include "OnlinePlayManager.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "TimerManager.h"

namespace {
void (*GameSetupCallback)() = nullptr;

bool HasCommandLineArgument(const std::wstring& Argument) {
  int ArgumentCount = 0;
  LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgumentCount);
  if (!Arguments) {
    return false;
  }

  bool bFound = false;
  for (int ArgumentIndex = 1; ArgumentIndex < ArgumentCount; ++ArgumentIndex) {
    if (Argument == Arguments[ArgumentIndex]) {
      bFound = true;
      break;
    }
  }
  LocalFree(Arguments);
  return bFound;
}
}  // namespace
extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ImGuiHookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
      msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
      msg == WM_RBUTTONDBLCLK || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP ||
      msg == WM_MBUTTONDBLCLK) {
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    lParam = MAKELPARAM(static_cast<WORD>(mx), static_cast<WORD>(my));
  }

  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return 1;
  }
  return 0;
}

void Application::SetGameSetupCallback(void (*Callback)()) { GameSetupCallback = Callback; }

Application::Application() : AutomationCommandQueue(std::make_unique<FAutomationCommandQueue>()) {}

Application::~Application() { Shutdown(); }

void Application::Shutdown() {
  ShutdownAutomation();

  if (bImGuiInitialized) {
    SetHookWinProc(nullptr);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    bImGuiInitialized = false;
    M_LOG("Application ImGui shutdown completed.");
  }

  if (OffscreenBuffer != -1) {
    DeleteGraph(OffscreenBuffer);
    OffscreenBuffer = -1;
    M_LOG("Application offscreen buffer released.");
  }

  if (bDxLibInitialized) {
    DxLib_End();
    bDxLibInitialized = false;
    M_LOG("DxLib_End completed.");
  }
}

void Application::ShutdownAutomation() {
  if (AutomationHttpServer) {
    AutomationHttpServer->StopAcceptingRequests();
  }

  if (AutomationCommandQueue) {
    AutomationCommandQueue->StopAcceptingCommands();
    AutomationCommandQueue->CancelAll(
        EAutomationErrorCode::EngineShuttingDown, "The engine is shutting down."
    );
  }

  if (AutomationHttpServer) {
    AutomationHttpServer->Stop();
  }

  AutomationHttpServer.reset();
  AutomationApiController.reset();
  AutomationCommandQueue.reset();
}

void Application::InitializeAutomation() {
  FAutomationConfig Config;
  Config.Enabled = HasCommandLineArgument(L"-automation");
  if (!Config.Enabled) {
    M_LOG("Automation disabled.");
    return;
  }

  if (!AutomationCommandQueue) {
    AutomationCommandQueue = std::make_unique<FAutomationCommandQueue>();
  }

  FAutomationStateProvider StateProvider = [this]() {
    nlohmann::json State = {
        {"sceneName", ""},
        {"fps", 0.0f},
        {"paused", bPosed},
        {"worldAvailable", false},
        {"actorCount", 0u}
    };

    SceneManager& Manager = SceneManager::GetInstance();
    World* CurrentWorld = Manager.GetCurrentScene();
    if (!CurrentWorld) {
      return State;
    }

    std::string SceneName;
    const std::string& LevelPath = Manager.GetCurrentLevelPath();
    if (!LevelPath.empty()) {
      SceneName = std::filesystem::path(LevelPath).stem().string();
    }

    State["sceneName"] = std::move(SceneName);
    State["fps"] = CurrentWorld->GetCurrentFps();
    State["worldAvailable"] = true;
    if (const FActorManager* ActorManager = CurrentWorld->GetActorManager()) {
      State["actorCount"] = ActorManager->GetActiveActorCount();
    }
    return State;
  };

  AutomationApiController = std::make_unique<FAutomationApiController>(
      *AutomationCommandQueue, Config, std::move(StateProvider)
  );
  AutomationHttpServer = std::make_unique<FAutomationHttpServer>(Config, *AutomationApiController);
  if (!AutomationHttpServer->Start()) {
    M_LOG("Automation server startup failed; the engine will continue without Automation.");
    AutomationHttpServer.reset();
    AutomationApiController.reset();
  }
}

void Application::InitOffscreenBuffer() {
  if (OffscreenBuffer != -1) {
    DeleteGraph(OffscreenBuffer);
  }
  OffscreenBuffer = MakeScreen(VirtualWidth, VirtualHeight, true);
}

void Application::SetWindowResolution(int width, int height) { SetWindowSize(width, height); }
static bool ShouldQuitGame = false;
void Application::QuitGame() { ShouldQuitGame = true; }

bool Application::Run() {
  SetProcessDPIAware();
  SetGraphMode(1920, 1080, 32);
  SetUseDirect3D11(true);
  bool bFitScreen = !IsEditor;
  bool bFullScreen = !IsRelease;
  SetWindowSizeChangeEnableFlag(true, bFitScreen);
  ChangeWindowMode(bFullScreen);
  SetDoubleStartValidFlag(TRUE);
  SetOutApplicationLogValidFlag(FALSE);
  SetAlwaysRunFlag(TRUE);
  SetWaitVSyncFlag(false);
  SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
  std::string mode;
  if (IsEditor) {
    mode = "Editor";
  } else if (IsRelease) {
    mode = "Release";
  } else {
    mode = "Game";
  }
  M_LOG("Starting: {}", mode);
  LONGLONG LastTime = GetNowHiPerformanceCount();

  if (DxLib_Init() == -1) {
    M_LOG("DxLib_Init failed.");
    return false;
  }
  bDxLibInitialized = true;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(GetMainWindowHandle());
  ImGui_ImplDX11_Init(
      reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device())),
      reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()))
  );
  SetHookWinProc(ImGuiHookProc);
  bImGuiInitialized = true;

  InitOffscreenBuffer();

  if (IsEditor) {
    SceneManager::GetInstance().OpenGameMode<EditorMode>();
  } else {
    if (GameSetupCallback) {
      GameSetupCallback();
    }
    SceneManager::GetInstance().OpenStartupLevel();
  }

  if (!IsRelease) {
    HWND hwnd = GetMainWindowHandle();
    SetWindowPos(hwnd, NULL, 0, 0, 960, 540, SWP_NOZORDER | SWP_SHOWWINDOW);
    SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
  }

  auto& IM = InputManager::GetInstance();
  IM.AddDevice(std::make_unique<KeyboardDevice>());
  IM.AddDevice(std::make_unique<MouseDevice>());
  IM.AddDevice(std::make_unique<GamepadDevice>(1));

  InitializeAutomation();

  while (ProcessMessage() == 0 && !ShouldQuitGame) {
    int TargetFps = 120;
    if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
      TargetFps = CurrentScene->GetTargetFps();
    }
    const LONGLONG TargetFrameTime = 1000000 / TargetFps;

    LONGLONG CurrentTime = GetNowHiPerformanceCount();
    LONGLONG ElapsedTime = CurrentTime - LastTime;
    if (ElapsedTime < TargetFrameTime) {
      LONGLONG SleepTime = (TargetFrameTime - ElapsedTime) / 1000;
      if (SleepTime > 0) Sleep((DWORD)SleepTime);
      CurrentTime = GetNowHiPerformanceCount();
      ElapsedTime = CurrentTime - LastTime;
    }
    DeltaTime = static_cast<float>(ElapsedTime) / 1000000.0f;
    LastTime = CurrentTime;

    if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
      CurrentScene->UpdateCurrentFps(DeltaTime);
    }

    if (DeltaTime > 0.1f) DeltaTime = 0.1f;
    Update(DeltaTime);
    Draw();
  }

  ShutdownAutomation();

  EOSTitleStorageManager::GetInstance().Shutdown();
  SceneManager::GetInstance().Shutdown();
  OnlinePlayManager::GetInstance().Shutdown();
  NetworkManager::GetInstance().Stop();
  EOSLobbyManager::GetInstance().Shutdown();
  EOSAuthManager::GetInstance().Shutdown();
  for (int TickIndex = 0; TickIndex < 3; ++TickIndex) {
    EOSCoreManager::GetInstance().Tick();
  }
  EOSCoreManager::GetInstance().Shutdown();
  Shutdown();

  return true;
}

bool Application::Update(float DeltaTime) {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGuiIO& io = ImGui::GetIO();

  int screenW, screenH;
  GetDrawScreenSize(&screenW, &screenH);
  io.DisplaySize = ImVec2((float)screenW, (float)screenH);

  int mx, my;
  GetMousePoint(&mx, &my);
  io.AddMousePosEvent((float)mx, (float)my);

  ImGui::NewFrame();

  SceneManager::GetInstance().ProcessSceneChanges();
  if (AutomationCommandQueue) {
    AutomationCommandQueue->ProcessCommands();
  }
  EOSCoreManager::GetInstance().Tick();
  NetworkManager::GetInstance().Service();
  InputManager::GetInstance().Update();
  HttpManager::GetInstance().Update();
#if !defined(_RELEASE)
  DebugOverlayManager::GetInstance().Update(DeltaTime);
#endif

  if (bPosed) return true;

  if (World* currentScene = SceneManager::GetInstance().GetCurrentScene()) {
    currentScene->Update(DeltaTime);
  }

  return true;
}

bool Application::Draw() {
  SetDrawScreen(OffscreenBuffer);
  ClearDrawScreen();

  if (World* currentScene = SceneManager::GetInstance().GetCurrentScene()) {
    currentScene->Draw();
  }
  RenderSystem::GetInstance().Draw();

  SetDrawScreen(DX_SCREEN_BACK);
  ClearDrawScreen();

  int screenW, screenH;
  GetDrawScreenSize(&screenW, &screenH);
  float scaleX = (float)screenW / VirtualWidth;
  float scaleY = (float)screenH / VirtualHeight;
  float scale = (scaleX < scaleY) ? scaleX : scaleY;

  int drawW = (int)(VirtualWidth * scale);
  int drawH = (int)(VirtualHeight * scale);
  int drawX = (screenW - drawW) / 2;
  int drawY = (screenH - drawH) / 2;

  SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
  DrawExtendGraph(drawX, drawY, drawX + drawW, drawY + drawH, OffscreenBuffer, false);
  SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

  SetDrawScreen(DX_SCREEN_BACK);
#if !defined(_RELEASE)
  DebugOverlayManager::GetInstance().Draw();
#endif
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  ScreenFlip();

  return true;
}

void* Application::GetImGuiContext() { return static_cast<void*>(ImGui::GetCurrentContext()); }
