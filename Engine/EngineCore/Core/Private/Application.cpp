#include "Application.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
#include <cmath>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "AutomationSubsystem.h"
#include "BroccoliRaylib.h"
#include "DebugOverlay.h"
#include "EditorMode.h"
#include "EngineDefine.h"
#include "GamePadDevice.h"
#include "HttpManager.h"
#include "InputManager.h"
#include "KeyboardDevice.h"
#include "Log.h"
#include "MouseDevice.h"
#include "NetworkManager.h"
#include "PathResolver.h"
#include "PerformanceOverlay.h"
#include "PluginHost.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Systems/Private/Jolt/JoltRuntime.h"
#include "TimerManager.h"
#include "World.h"
#include "rlImGui/rlImGui.h"

namespace {
void (*GameSetupCallback)() = nullptr;
bool ShouldQuitGame = false;
Application* CurrentApplication = nullptr;
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

std::wstring CommandLineValue(const std::wstring& Argument) {
  int ArgumentCount = 0;
  LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgumentCount);
  if (!Arguments) return {};
  std::wstring Result;
  for (int Index = 1; Index < ArgumentCount; ++Index) {
    if (Argument == Arguments[Index]) {
      Result = Index + 1 < ArgumentCount ? Arguments[Index + 1] : L"";
      break;
    }
  }
  LocalFree(Arguments);
  return Result;
}

bool HasCommandLineArgument(const std::wstring& Argument) {
  int ArgumentCount = 0;
  LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgumentCount);
  if (!Arguments) return false;
  bool Found = false;
  for (int Index = 1; Index < ArgumentCount; ++Index) {
    if (Argument == Arguments[Index]) Found = true;
  }
  LocalFree(Arguments);
  return Found;
}

RenderTexture2D* AsRenderTexture(void* Buffer) { return static_cast<RenderTexture2D*>(Buffer); }
}  // namespace

void Application::SetGameSetupCallback(void (*Callback)()) { GameSetupCallback = Callback; }

Application::Application() { CurrentApplication = this; }

Application::~Application() {
  Shutdown();
  if (CurrentApplication == this) CurrentApplication = nullptr;
}

FEngineUpdateControl* Application::GetUpdateControl() {
  return CurrentApplication ? &CurrentApplication->UpdateControl : nullptr;
}

void Application::Shutdown() {
  RemoveLiveResizeHook();
  UpdateControl.Shutdown();

  NetworkManager::GetInstance().Stop();
  PluginHost::GetInstance().Shutdown();

  if (AutomationSubsystem) {
    AutomationSubsystem->Shutdown();
    AutomationSubsystem.reset();
  }

  if (ImGuiInitialized) {
    rlImGuiShutdown();
    ImGuiInitialized = false;
    M_LOG(Log, "Application ImGui shutdown completed.");
  }

  if (OffscreenBuffer != nullptr) {
    RenderTexture2D* Buffer = AsRenderTexture(OffscreenBuffer);
    UnloadRenderTexture(*Buffer);
    delete Buffer;
    OffscreenBuffer = nullptr;
    M_LOG(Log, "Application offscreen buffer released.");
  }

  if (RaylibInitialized) {
    ResourceManager::GetInstance().ReleaseAllResources();
  }

  if (AudioInitialized) {
    CloseAudioDevice();
    AudioInitialized = false;
  }

  if (RaylibInitialized) {
    CloseWindow();
    RaylibInitialized = false;
    M_LOG(Log, "raylib shutdown completed.");
  }

  MLog::Shutdown();
}

void Application::InitializeAutomation() {
  AutomationSubsystem = std::make_unique<FAutomationSubsystem>();
  FAutomationConfig Config;
  Config.Enabled = HasCommandLineArgument(L"--control");
  if (!AutomationSubsystem->Initialize(Config)) AutomationSubsystem.reset();
}

bool Application::InitOffscreenBuffer(int Width, int Height) {
  if (Width <= 0 || Height <= 0) return false;

  if (OffscreenBuffer != nullptr) {
    const RenderTexture2D* ExistingBuffer = AsRenderTexture(OffscreenBuffer);
    if (ExistingBuffer->texture.width == Width && ExistingBuffer->texture.height == Height) {
      RenderSystem::GetInstance().SetRenderTargetSize(Width, Height);
      return true;
    }
  }

  auto* Buffer = new RenderTexture2D(LoadRenderTexture(Width, Height));
  if (!IsRenderTextureValid(*Buffer)) {
    delete Buffer;
    return false;
  }
  SetTextureFilter(Buffer->texture, TEXTURE_FILTER_BILINEAR);

  if (OffscreenBuffer != nullptr) {
    RenderTexture2D* ExistingBuffer = AsRenderTexture(OffscreenBuffer);
    UnloadRenderTexture(*ExistingBuffer);
    delete ExistingBuffer;
  }
  OffscreenBuffer = Buffer;
  RenderSystem::GetInstance().SetRenderTargetSize(Width, Height);
  return true;
}

void Application::SetWindowResolution(int Width, int Height) {
  if (IsWindowReady() && Width > 0 && Height > 0) SetWindowSize(Width, Height);
}

void Application::QuitGame() {
  ShouldQuitGame = true;
  if (auto* Control = GetUpdateControl()) Control->RequestShutdown();
}

bool Application::Run() {
  PathResolver::InitializeWorkingDirectory();
  MLog::Initialize();
  const std::filesystem::path ExecutableDirectory = PathResolver::GetExecutableDirectory();
  if (ExecutableDirectory.empty()) {
    M_LOG(Error, "Could not determine the executable directory.");
    Shutdown();
    return false;
  } else {
    if (!PluginHost::GetInstance().Initialize(ExecutableDirectory / "plugins.json")) {
      Shutdown();
      return false;
    }
  }
  const std::wstring EngineUpdate = CommandLineValue(L"--engine-update");
  if (HasCommandLineArgument(L"--engine-update") && EngineUpdate != L"stopped" &&
      EngineUpdate != L"running") {
    M_LOG(Error, "--engine-update must be running or stopped.");
    return false;
  }
  float FixedDeltaTime = 1.0f / 60.0f;
  if (HasCommandLineArgument(L"--step-delta-time")) {
    try {
      const std::wstring Value = CommandLineValue(L"--step-delta-time");
      size_t Parsed = 0;
      FixedDeltaTime = std::stof(Value, &Parsed);
      if (Parsed != Value.size() || !std::isfinite(FixedDeltaTime) || FixedDeltaTime <= 0.0f ||
          FixedDeltaTime > 0.1f)
        throw std::invalid_argument("step delta time");
    } catch (...) {
      M_LOG(Error, "--step-delta-time must be finite, positive and at most 0.1 seconds.");
      return false;
    }
  }
  UpdateControl.Configure(EngineUpdate != L"stopped", FixedDeltaTime);
  SetProcessDPIAware();
  SetConfigFlags(
      FLAG_WINDOW_RESIZABLE | (HasCommandLineArgument(L"--window-hidden") ? FLAG_WINDOW_HIDDEN : 0)
  );

  std::string Mode;
  if (IsEditor) {
    Mode = "Editor";
  } else if (IsRelease) {
    Mode = "Release";
  } else {
    Mode = "Game";
  }
  M_LOG(Log, "Starting: {}", Mode);

  const std::string WindowTitle = "BroccoliEngine - " + Mode;
  InitWindow(1920, 1080, WindowTitle.c_str());
  if (!IsWindowReady()) {
    M_LOG(Error, "raylib InitWindow failed.");
    return false;
  }
  RaylibInitialized = true;
  ShouldQuitGame = false;
  SetExitKey(KEY_NULL);

  if (IsRelease && !HasCommandLineArgument(L"--window-hidden")) {
    ToggleFullscreen();
  } else {
    SetWindowSize(1920, 1080);
    SetWindowPosition(10, 100);
    SetWindowResolution(960, 540);
  }

  InitAudioDevice();
  AudioInitialized = IsAudioDeviceReady();
  if (!AudioInitialized) M_LOG(Warning, "raylib InitAudioDevice failed.");

  rlImGuiSetup(true);
  ImGuiInitialized = ImGui::GetCurrentContext() != nullptr;
  if (ImGuiInitialized) {
    ImGuiIO& Io = ImGui::GetIO();
    Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  }
  if (!InitOffscreenBuffer(VirtualWidth, VirtualHeight)) {
    M_LOG(Error, "raylib LoadRenderTexture failed for the virtual screen.");
    Shutdown();
    return false;
  }

  ResourceManager::GetInstance();
  if (!FJoltRuntime::Initialize()) {
    M_LOG(Error, "Jolt runtime initialization failed.");
    Shutdown();
    return false;
  }
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
  ProcessSceneChanges();
  InitializeAutomation();

  if (!InstallLiveResizeHook([this](float ResizeDeltaTime) {
        Tick(ResizeDeltaTime, false, false);
      })) {
    M_LOG(Warning, "Live window resize redraw hook installation failed.");
  }

  while (!WindowShouldClose() && !ShouldQuitGame) {
    int TargetFps = 120;
    if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
      TargetFps = CurrentScene->GetTargetFps();
    }
    SetTargetFPS((std::max)(1, TargetFps));

    Tick((std::min)(GetFrameTime(), 0.1f), true, true);
    MLog::EndFrame();
  }

  RemoveLiveResizeHook();
  UpdateControl.Shutdown();

  if (AutomationSubsystem) {
    AutomationSubsystem->Shutdown();
    AutomationSubsystem.reset();
  }
  SceneManager::GetInstance().Shutdown();
  FJoltRuntime::Shutdown();
  Shutdown();
  return true;
}

void Application::ControlUpdate() {
  if (AutomationSubsystem) AutomationSubsystem->Update();
}

bool Application::ProcessSceneChanges() {
  auto& Manager = SceneManager::GetInstance();
  UpdateControl.FailSupersededLevel(Manager.GetPendingRequestId());
  const auto Result = Manager.ProcessSceneChanges();
  World* CurrentScene = SceneManager::GetInstance().GetCurrentScene();
  if (CurrentScene != nullptr) {
    if (auto* CurrentEditorMode = dynamic_cast<EditorMode*>(CurrentScene->GetGameMode())) {
      const FVector2D RequestedSize = CurrentEditorMode->GetViewportState().RequestedRenderSize;
      if (RequestedSize.X > 0.0f && RequestedSize.Y > 0.0f) {
        constexpr int ResizeQuantum = 4;
        const int RequestedWidth =
            (std::max)(ResizeQuantum,
                       static_cast<int>(std::lround(RequestedSize.X / ResizeQuantum)) *
                           ResizeQuantum);
        const int RequestedHeight =
            (std::max)(ResizeQuantum,
                       static_cast<int>(std::lround(RequestedSize.Y / ResizeQuantum)) *
                           ResizeQuantum);
        if (!InitOffscreenBuffer(RequestedWidth, RequestedHeight)) {
          M_LOG(
              Warning,
              "Viewport render texture resize failed: {}x{}",
              RequestedWidth,
              RequestedHeight
          );
        }
      }

      const RenderTexture2D* Buffer = AsRenderTexture(OffscreenBuffer);
      CurrentEditorMode->SetViewportRenderTexture(
          OffscreenBuffer, Buffer->texture.width, Buffer->texture.height
      );
    }
  }
  return Result.RequestId != 0 && UpdateControl.OnSceneProcessed(Result.RequestId, Result.Success);
}

void Application::Tick(float FrameDeltaTime, bool ProcessInput, bool CompleteFrame) {
  if (TickInProgress || ShouldQuitGame) return;
  TickInProgress = true;
  if (!UpdateControl.BeginUpdate()) {
    ControlUpdate();
    // EndDrawing normally polls events and throttles. Neither runs while stopped.
    if (CompleteFrame) {
      TickInProgress = false;
      PollInputEvents();
      TickInProgress = true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    TickInProgress = false;
    return;
  }
  if (ProcessSceneChanges()) {
    UpdateControl.SkipUpdate();
    TickInProgress = false;
    return;
  }
  DeltaTime =
      UpdateControl.IsStepUpdate()
          ? UpdateControl.GetStepDeltaTime()
          : (UpdateControl.TakeTimeReset() ? UpdateControl.GetStepDeltaTime() : FrameDeltaTime);
  if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
    CurrentScene->UpdateCurrentFps(DeltaTime);
  }
#if !defined(_RELEASE)
  auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
  PerformanceOverlay.BeginFrame();
  PerformanceOverlay.BeginUpdate();
#endif
  EngineUpdate(DeltaTime, ProcessInput);
  UpdateControl.CompleteUpdate();
#if !defined(_RELEASE)
  PerformanceOverlay.EndUpdate();
  PerformanceOverlay.BeginRender();
#endif
  Draw(CompleteFrame);
  TickInProgress = false;
}

bool Application::EngineUpdate(float FrameDeltaTime, bool ProcessInput) {
#if !defined(_RELEASE)
  auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
#endif
  if (ImGuiInitialized) rlImGuiBeginDelta(FrameDeltaTime);
#if !defined(_RELEASE)
  PerformanceOverlay.BeginSection(EPerformanceSection::Plugins);
#endif
  PluginHost::GetInstance().Update(FrameDeltaTime);
#if !defined(_RELEASE)
  PerformanceOverlay.EndSection(EPerformanceSection::Plugins);
#endif

  World* CurrentScene = SceneManager::GetInstance().GetCurrentScene();
  if (AutomationSubsystem) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::Automation);
#endif
    ControlUpdate();
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::Automation);
#endif
  }
#if !defined(_RELEASE)
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

  if (CurrentScene != nullptr && CurrentScene->GetSoundManager() != nullptr) {
#if !defined(_RELEASE)
    PerformanceOverlay.BeginSection(EPerformanceSection::Audio);
#endif
    CurrentScene->GetSoundManager()->Update();
#if !defined(_RELEASE)
    PerformanceOverlay.EndSection(EPerformanceSection::Audio);
#endif
  }
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

  if (!IsEditor) {
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
  }

#if !defined(_RELEASE)
  DebugOverlayManager::GetInstance().Draw();
  PerformanceOverlayManager::GetInstance().Draw();
#endif
  if (ImGuiInitialized) rlImGuiEnd();
#if !defined(_RELEASE)
  auto& PerformanceOverlay = PerformanceOverlayManager::GetInstance();
  PerformanceOverlay.EndRender();
  PerformanceOverlay.EndFrame();

  int TargetFps = 0;
  if (World* CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
    TargetFps = CurrentScene->GetTargetFps();
  }
  PerformanceOverlay.CommitFrame(TargetFps);
  PerformanceOverlay.Update(DeltaTime);
#endif
  if (CompleteFrame) {
    // The update, render, ImGui and performance frame are closed. Windows may enter a
    // modal resize loop here; allow its timer to start another complete controlled tick.
    TickInProgress = false;
    EndDrawing();
    TickInProgress = true;
  } else {
    rlDrawRenderBatchActive();
    SwapScreenBuffer();
  }
  return true;
}

void* Application::GetImGuiContext() { return static_cast<void*>(ImGui::GetCurrentContext()); }
