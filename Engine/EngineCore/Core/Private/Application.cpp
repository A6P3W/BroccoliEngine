#include "Application.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#include <Windows.h>
#include <shellapi.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <utility>

#include "AutomationSubsystem.h"
#include "BroccoliRaylib.h"
#include "DebugOverlay.h"
#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "EOSTitleStorageManager.h"
#include "EditorMode.h"
#include "EngineDefine.h"
#include "GamePadDevice.h"
#include "HttpManager.h"
#include "InputManager.h"
#include "KeyboardDevice.h"
#include "Log.h"
#include "MouseDevice.h"
#include "NetworkManager.h"
#include "OnlinePlayManager.h"
#include "PathResolver.h"
#include "PerformanceOverlay.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "World.h"
#include "rlImGui/rlImGui.h"

namespace {
void (*GameSetupCallback)() = nullptr;
bool ShouldQuitGame = false;
constexpr UINT_PTR LiveResizeTimerId = 0xB0CC011;
constexpr UINT LiveResizeIntervalMilliseconds = 16;
HWND LiveResizeWindowHandle = nullptr;
WNDPROC PreviousWindowProcedure = nullptr;
std::function<void(float)> LiveResizeTickCallback;
std::chrono::steady_clock::time_point LastLiveResizeTick;
bool LiveResizeTickInProgress = false;

void TickDuringLiveResize() {
  if (!LiveResizeTickCallback || LiveResizeTickInProgress || IsWindowMinimized()) return;

  const auto CurrentTick = std::chrono::steady_clock::now();
  const float ResizeDeltaTime =
      (std::min)(std::chrono::duration<float>(CurrentTick - LastLiveResizeTick).count(), 0.1f);
  LastLiveResizeTick = CurrentTick;
  if (ResizeDeltaTime <= 0.0f) return;

  LiveResizeTickInProgress = true;
  LiveResizeTickCallback(ResizeDeltaTime);
  LiveResizeTickInProgress = false;
}

LRESULT CALLBACK
LiveResizeWindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
  const LRESULT Result =
      CallWindowProcW(PreviousWindowProcedure, WindowHandle, Message, WParam, LParam);

  switch (Message) {
    case WM_ENTERSIZEMOVE:
      LastLiveResizeTick = std::chrono::steady_clock::now();
      SetTimer(WindowHandle, LiveResizeTimerId, LiveResizeIntervalMilliseconds, nullptr);
      break;
    case WM_TIMER:
      if (WParam == LiveResizeTimerId) TickDuringLiveResize();
      break;
    case WM_EXITSIZEMOVE:
      KillTimer(WindowHandle, LiveResizeTimerId);
      TickDuringLiveResize();
      break;
    default:
      break;
  }
  return Result;
}

bool InstallLiveResizeHook(std::function<void(float)> Callback) {
  if (PreviousWindowProcedure != nullptr) return true;

  HWND WindowHandle = static_cast<HWND>(GetWindowHandle());
  if (WindowHandle == nullptr) return false;

  SetLastError(ERROR_SUCCESS);
  const LONG_PTR PreviousProcedure = SetWindowLongPtrW(
      WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&LiveResizeWindowProcedure)
  );
  if (PreviousProcedure == 0 && GetLastError() != ERROR_SUCCESS) return false;

  LiveResizeWindowHandle = WindowHandle;
  PreviousWindowProcedure = reinterpret_cast<WNDPROC>(PreviousProcedure);
  LiveResizeTickCallback = std::move(Callback);
  return true;
}

void RemoveLiveResizeHook() {
  if (LiveResizeWindowHandle != nullptr) {
    KillTimer(LiveResizeWindowHandle, LiveResizeTimerId);
    if (PreviousWindowProcedure != nullptr) {
      SetWindowLongPtrW(
          LiveResizeWindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(PreviousWindowProcedure)
      );
    }
  }
  LiveResizeTickCallback = {};
  PreviousWindowProcedure = nullptr;
  LiveResizeWindowHandle = nullptr;
  LiveResizeTickInProgress = false;
}

bool HasCommandLineArgument(const std::wstring& Argument) {
  int ArgumentCount = 0;
  LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgumentCount);
  if (Arguments == nullptr) return false;

  bool Found = false;
  for (int ArgumentIndex = 1; ArgumentIndex < ArgumentCount; ++ArgumentIndex) {
    if (Argument == Arguments[ArgumentIndex]) {
      Found = true;
      break;
    }
  }
  LocalFree(Arguments);
  return Found;
}

RenderTexture2D* AsRenderTexture(void* Buffer) { return static_cast<RenderTexture2D*>(Buffer); }
}  // namespace

void Application::SetGameSetupCallback(void (*Callback)()) { GameSetupCallback = Callback; }

Application::Application() = default;

Application::~Application() { Shutdown(); }

void Application::Shutdown() {
  RemoveLiveResizeHook();

  if (AutomationSubsystem) {
    AutomationSubsystem->Shutdown();
    AutomationSubsystem.reset();
  }

  if (ImGuiInitialized) {
    rlImGuiShutdown();
    ImGuiInitialized = false;
    M_LOG("Application ImGui shutdown completed.");
  }

  if (OffscreenBuffer != nullptr) {
    RenderTexture2D* Buffer = AsRenderTexture(OffscreenBuffer);
    UnloadRenderTexture(*Buffer);
    delete Buffer;
    OffscreenBuffer = nullptr;
    M_LOG("Application offscreen buffer released.");
  }

  if (RaylibInitialized) {
    ResourceManager::GetInstance().ReleaseResourceGraph();
  }

  if (AudioInitialized) {
    CloseAudioDevice();
    AudioInitialized = false;
  }

  if (RaylibInitialized) {
    CloseWindow();
    RaylibInitialized = false;
    M_LOG("raylib shutdown completed.");
  }
}

void Application::InitializeAutomation() {
  AutomationSubsystem = std::make_unique<FAutomationSubsystem>();
  FAutomationConfig Config;
  Config.Enabled = HasCommandLineArgument(L"-automation");
  if (!AutomationSubsystem->Initialize(Config)) AutomationSubsystem.reset();
}

void Application::InitOffscreenBuffer() {
  if (OffscreenBuffer != nullptr) {
    RenderTexture2D* ExistingBuffer = AsRenderTexture(OffscreenBuffer);
    UnloadRenderTexture(*ExistingBuffer);
    delete ExistingBuffer;
  }

  auto* Buffer = new RenderTexture2D(LoadRenderTexture(VirtualWidth, VirtualHeight));
  if (!IsRenderTextureValid(*Buffer)) {
    delete Buffer;
    OffscreenBuffer = nullptr;
    return;
  }
  SetTextureFilter(Buffer->texture, TEXTURE_FILTER_BILINEAR);
  OffscreenBuffer = Buffer;
}

void Application::SetWindowResolution(int Width, int Height) {
  if (IsWindowReady() && Width > 0 && Height > 0) SetWindowSize(Width, Height);
}

void Application::QuitGame() { ShouldQuitGame = true; }

bool Application::Run() {
  PathResolver::InitializeWorkingDirectory();
  SetProcessDPIAware();
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  std::string Mode;
  if (IsEditor) {
    Mode = "Editor";
  } else if (IsRelease) {
    Mode = "Release";
  } else {
    Mode = "Game";
  }
  M_LOG("Starting: {}", Mode);

  const std::string WindowTitle = "BroccoliEngine - " + Mode;
  InitWindow(1920, 1080, WindowTitle.c_str());
  if (!IsWindowReady()) {
    M_LOG("raylib InitWindow failed.");
    return false;
  }
  RaylibInitialized = true;
  ShouldQuitGame = false;
  SetExitKey(KEY_NULL);

  if (IsRelease) {
    ToggleFullscreen();
  } else {
    SetWindowSize(1920, 1080);
    SetWindowPosition(10, 100);
    SetWindowResolution(960, 540);
  }

  InitAudioDevice();
  AudioInitialized = IsAudioDeviceReady();
  if (!AudioInitialized) M_LOG("raylib InitAudioDevice failed.");

  rlImGuiSetup(true);
  ImGuiInitialized = ImGui::GetCurrentContext() != nullptr;
  InitOffscreenBuffer();
  if (OffscreenBuffer == nullptr) {
    M_LOG("raylib LoadRenderTexture failed for the virtual screen.");
    Shutdown();
    return false;
  }

  ResourceManager::GetInstance();
  if (IsEditor) {
    SceneManager::GetInstance().OpenGameMode<EditorMode>();
  } else {
    if (GameSetupCallback != nullptr) GameSetupCallback();
    SceneManager::GetInstance().OpenStartupLevel();
  }

  auto& Input = InputManager::GetInstance();
  Input.AddDevice(std::make_unique<KeyboardDevice>());
  Input.AddDevice(std::make_unique<MouseDevice>());
  Input.AddDevice(std::make_unique<GamepadDevice>(1));
  InitializeAutomation();

  if (!InstallLiveResizeHook([this](float ResizeDeltaTime) {
        DeltaTime = ResizeDeltaTime;
        if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
          CurrentScene->UpdateCurrentFps(DeltaTime);
        }
        Update(DeltaTime, false);
        Draw(false);
      })) {
    M_LOG("Live window resize redraw hook installation failed.");
  }

  while (!WindowShouldClose() && !ShouldQuitGame) {
    int TargetFps = 120;
    if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
      TargetFps = CurrentScene->GetTargetFps();
    }
    SetTargetFPS((std::max)(1, TargetFps));

    DeltaTime = (std::min)(GetFrameTime(), 0.1f);
    if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
      CurrentScene->UpdateCurrentFps(DeltaTime);
    }

#if !defined(_RELEASE)
    auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
    PerformanceOverlay.BeginUpdate();
#endif
    Update(DeltaTime, true);
#if !defined(_RELEASE)
    PerformanceOverlay.EndUpdate();

    PerformanceOverlay.BeginRender();
#endif
    Draw(true);
  }

  RemoveLiveResizeHook();

  if (AutomationSubsystem) {
    AutomationSubsystem->Shutdown();
    AutomationSubsystem.reset();
  }
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

bool Application::Update(float FrameDeltaTime, bool ProcessInput) {
#if !defined(_RELEASE)
  auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
#endif
  if (ImGuiInitialized) rlImGuiBeginDelta(FrameDeltaTime);

#if !defined(_RELEASE)
  PerformanceOverlay.BeginSection(EPerformanceSection::Scene);
#endif
  SceneManager::GetInstance().ProcessSceneChanges();
#if !defined(_RELEASE)
  PerformanceOverlay.EndSection(EPerformanceSection::Scene);
#endif
  if (AutomationSubsystem) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::Automation);
#endif
    AutomationSubsystem->Update();
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::Automation);
#endif
  }
#if !defined(_RELEASE)
  PerformanceOverlay.BeginSection(EPerformanceSection::EOS);
#endif
  EOSCoreManager::GetInstance().Tick();
#if !defined(_RELEASE)
  PerformanceOverlay.EndSection(EPerformanceSection::EOS);
  PerformanceOverlay.BeginSection(EPerformanceSection::Network);
#endif
  NetworkManager::GetInstance().Service();
#if !defined(_RELEASE)
  PerformanceOverlay.EndSection(EPerformanceSection::Network);
#endif
  if (ProcessInput) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::Input);
#endif
    InputManager::GetInstance().Update();
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::Input);
#endif
  }
#if !defined(_RELEASE)
  PerformanceOverlay.BeginSection(EPerformanceSection::Http);
#endif
  HttpManager::GetInstance().Update();
#if !defined(_RELEASE)
  PerformanceOverlay.EndSection(EPerformanceSection::Http);
  PerformanceOverlay.BeginSection(EPerformanceSection::DebugOverlay);
  DebugOverlayManager::GetInstance().Update(FrameDeltaTime);
  PerformanceOverlay.EndSection(EPerformanceSection::DebugOverlay);
#endif

  World* CurrentScene = SceneManager::GetInstance().GetCurrentScene();
  if (CurrentScene != nullptr && CurrentScene->GetSoundManager() != nullptr) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::Audio);
#endif
    CurrentScene->GetSoundManager()->Update();
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::Audio);
#endif
  }
  if (AutomationSubsystem && AutomationSubsystem->IsPaused()) return true;
  if (CurrentScene != nullptr) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::World);
#endif
    CurrentScene->Update(FrameDeltaTime);
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::World);
#endif
  }
  return true;
}

bool Application::Draw(bool CompleteFrame) {
  RenderTexture2D* Buffer = AsRenderTexture(OffscreenBuffer);
  if (Buffer == nullptr) return false;

  BeginTextureMode(*Buffer);
  ClearBackground(BLANK);
  rlSetBlendFactorsSeparate(
      RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD
  );
  BeginBlendMode(BLEND_CUSTOM_SEPARATE);
  if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
    CurrentScene->Draw();
  }
  RenderSystem::GetInstance().Draw();
  EndBlendMode();
  EndTextureMode();

  BeginDrawing();
  ClearBackground(BLACK);

  const int ScreenWidth = GetScreenWidth();
  const int ScreenHeight = GetScreenHeight();
  const float ScaleX = static_cast<float>(ScreenWidth) / VirtualWidth;
  const float ScaleY = static_cast<float>(ScreenHeight) / VirtualHeight;
  const float Scale = (std::min)(ScaleX, ScaleY);
  const float DrawWidth = VirtualWidth * Scale;
  const float DrawHeight = VirtualHeight * Scale;
  const Rectangle Source = {
      0.0f,
      0.0f,
      static_cast<float>(VirtualWidth),
      -static_cast<float>(VirtualHeight),
  };
  const Rectangle Destination = {
      (ScreenWidth - DrawWidth) * 0.5f,
      (ScreenHeight - DrawHeight) * 0.5f,
      DrawWidth,
      DrawHeight,
  };
  BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
  DrawTexturePro(Buffer->texture, Source, Destination, {0.0f, 0.0f}, 0.0f, WHITE);
  EndBlendMode();

#if !defined(_RELEASE)
  DebugOverlayManager::GetInstance().Draw();
  PerformanceOverlayManager::GetInstance().Draw();
#endif
  if (ImGuiInitialized) rlImGuiEnd();
#if !defined(_RELEASE)
  auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
  PerformanceOverlay.EndRender();

  int TargetFps = 0;
  if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
    TargetFps = CurrentScene->GetTargetFps();
  }
  PerformanceOverlay.CommitFrame(TargetFps);
  PerformanceOverlay.Update(DeltaTime);
#endif
  if (CompleteFrame) {
    EndDrawing();
  } else {
    rlDrawRenderBatchActive();
    SwapScreenBuffer();
  }
  return true;
}

void* Application::GetImGuiContext() { return static_cast<void*>(ImGui::GetCurrentContext()); }
