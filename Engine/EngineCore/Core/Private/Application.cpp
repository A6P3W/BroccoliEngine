#include "Application.h"

#include <Windows.h>
#include <imgui.h>
#include <imgui_impl/imgui_impl_dx11.h>
#include <imgui_impl/imgui_impl_win32.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

#include "ActorManager.h"
#include "ActorRegistry.h"
#include "AutomationApiController.h"
#include "AutomationAutoRegistrar.h"
#include "AutomationCommandQueue.h"
#include "AutomationHttpServer.h"
#include "AutomationMethodRegistry.h"
#include "AutomationSystemCommandRegistry.h"
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
void (*AutomationMethodRegistrationCallback)(FAutomationMethodRegistry&) = nullptr;

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

std::string GetCurrentSceneName(const SceneManager& Manager) {
  const std::string& LevelPath = Manager.GetCurrentLevelPath();
  return LevelPath.empty() ? std::string() : std::filesystem::path(LevelPath).stem().string();
}

EAutomationWorldReadStatus MakeActorSnapshot(
    AActor& Actor, World& CurrentWorld, FAutomationActorSnapshot& OutSnapshot
) {
  if (Actor.IsPendingDestroy() || Actor.GetWorld() != &CurrentWorld) {
    return EAutomationWorldReadStatus::ActorNotFound;
  }

  FAutomationActorSnapshot Snapshot;
  Snapshot.ActorId = Actor.GetActorId();
  Snapshot.InstanceName = Actor.GetInstanceName();
  Snapshot.ClassName = Actor.GetActorClassName();
  Snapshot.Location = Actor.GetActorLocation();
  Snapshot.Rotation = Actor.GetActorRotation();
  Snapshot.Scale = Actor.GetActorScale();

  if (Snapshot.ActorId == InvalidActorId || Snapshot.InstanceName.empty() ||
      Snapshot.ClassName.empty() || !std::isfinite(Snapshot.Location.X) ||
      !std::isfinite(Snapshot.Location.Y) || !std::isfinite(Snapshot.Rotation.Rotation) ||
      !std::isfinite(Snapshot.Scale.Scale)) {
    return EAutomationWorldReadStatus::InvalidState;
  }

  OutSnapshot = std::move(Snapshot);
  return EAutomationWorldReadStatus::Success;
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

void Application::SetAutomationMethodRegistrationCallback(
    void (*Callback)(FAutomationMethodRegistry&)
) {
  AutomationMethodRegistrationCallback = Callback;
}

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
  AutomationSystemCommandRegistry.reset();
  AutomationMethodRegistry.reset();
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
  AutomationMethodRegistry = std::make_unique<FAutomationMethodRegistry>();
  try {
    FAutomationAutoRegistrar::GetInstance().RegisterAll(*AutomationMethodRegistry);
    if (AutomationMethodRegistrationCallback) {
      AutomationMethodRegistrationCallback(*AutomationMethodRegistry);
    }
    AutomationMethodRegistry->Freeze();
  } catch (const std::exception& Exception) {
    M_LOG("Automation actor method registration failed: {}", Exception.what());
    AutomationMethodRegistry.reset();
    return;
  } catch (...) {
    M_LOG("Automation actor method registration failed with an unknown exception.");
    AutomationMethodRegistry.reset();
    return;
  }

  AutomationSystemCommandRegistry = std::make_unique<FAutomationSystemCommandRegistry>();
  FAutomationSystemCommandDescriptor PauseDescriptor;
  PauseDescriptor.Name = "pause_game";
  PauseDescriptor.Description = "Pause world updates while keeping automation available.";
  PauseDescriptor.Handler = [this](const nlohmann::json&) {
    const bool Changed = !bPaused;
    bPaused = true;
    M_LOG(
        "Automation system command state changed: command=pause_game "
        "changed={} paused=true",
        Changed
    );
    return nlohmann::json{{"commandName", "pause_game"}, {"changed", Changed}, {"paused", true}};
  };

  FAutomationSystemCommandDescriptor ResumeDescriptor;
  ResumeDescriptor.Name = "resume_game";
  ResumeDescriptor.Description = "Resume world updates.";
  ResumeDescriptor.Handler = [this](const nlohmann::json&) {
    const bool Changed = bPaused;
    bPaused = false;
    M_LOG(
        "Automation system command state changed: command=resume_game "
        "changed={} paused=false",
        Changed
    );
    return nlohmann::json{{"commandName", "resume_game"}, {"changed", Changed}, {"paused", false}};
  };

  std::string RegistrationError;
  if (!AutomationSystemCommandRegistry->RegisterCommand(
          std::move(PauseDescriptor), &RegistrationError
      ) ||
      !AutomationSystemCommandRegistry->RegisterCommand(
          std::move(ResumeDescriptor), &RegistrationError
      )) {
    M_LOG("Automation system command registration failed: {}", RegistrationError);
    AutomationSystemCommandRegistry.reset();
    AutomationMethodRegistry.reset();
    return;
  }
  AutomationSystemCommandRegistry->Freeze();

  FAutomationStateProvider StateProvider = [this]() {
    nlohmann::json State = {
        {"sceneName", ""},
        {"fps", 0.0f},
        {"paused", bPaused},
        {"worldAvailable", false},
        {"actorCount", 0u}
    };

    SceneManager& Manager = SceneManager::GetInstance();
    World* CurrentWorld = Manager.GetCurrentScene();
    if (!CurrentWorld) {
      return State;
    }

    State["sceneName"] = GetCurrentSceneName(Manager);
    State["fps"] = CurrentWorld->GetCurrentFps();
    State["worldAvailable"] = true;
    if (const FActorManager* ActorManager = CurrentWorld->GetActorManager()) {
      State["actorCount"] = ActorManager->GetActiveActorCount();
    }
    return State;
  };

  FAutomationActorListProvider ActorListProvider = [](FAutomationActorListSnapshot& OutSnapshot) {
    SceneManager& Manager = SceneManager::GetInstance();
    World* CurrentWorld = Manager.GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return EAutomationWorldReadStatus::WorldNotAvailable;
    }

    FActorManager* ActorManager = CurrentWorld->GetActorManager();
    if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
      return EAutomationWorldReadStatus::InvalidState;
    }

    FAutomationActorListSnapshot Snapshot;
    Snapshot.SceneName = GetCurrentSceneName(Manager);
    for (const std::unique_ptr<AActor>& Actor : ActorManager->GetAllActors()) {
      if (!Actor || Actor->IsPendingDestroy()) {
        continue;
      }

      FAutomationActorSnapshot ActorSnapshot;
      const EAutomationWorldReadStatus Status =
          MakeActorSnapshot(*Actor, *CurrentWorld, ActorSnapshot);
      if (Status != EAutomationWorldReadStatus::Success) {
        return EAutomationWorldReadStatus::InvalidState;
      }
      Snapshot.Actors.push_back(std::move(ActorSnapshot));
    }

    std::sort(
        Snapshot.Actors.begin(),
        Snapshot.Actors.end(),
        [](const FAutomationActorSnapshot& Left, const FAutomationActorSnapshot& Right) {
          return Left.ActorId < Right.ActorId;
        }
    );
    OutSnapshot = std::move(Snapshot);
    return EAutomationWorldReadStatus::Success;
  };

  FAutomationActorProvider ActorProvider = [](FActorId ActorId,
                                              FAutomationActorSnapshot& OutSnapshot) {
    World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return EAutomationWorldReadStatus::WorldNotAvailable;
    }

    FActorManager* ActorManager = CurrentWorld->GetActorManager();
    if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
      return EAutomationWorldReadStatus::InvalidState;
    }

    AActor* Actor = ActorManager->FindActorById(ActorId);
    if (!Actor) {
      return EAutomationWorldReadStatus::ActorNotFound;
    }
    return MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot);
  };

  FAutomationSpawnActorProvider SpawnActorProvider = [](const FAutomationSpawnActorRequest& Request,
                                                        FAutomationActorSnapshot& OutSnapshot) {
    World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return EAutomationWorldMutationStatus::WorldNotAvailable;
    }

    FActorManager* ActorManager = CurrentWorld->GetActorManager();
    if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
      return EAutomationWorldMutationStatus::InvalidState;
    }

    ActorRegistry& Registry = ActorRegistry::GetInstance();
    if (!Registry.Contains(Request.ClassName)) {
      return EAutomationWorldMutationStatus::ClassNotRegistered;
    }

    AActor* Actor =
        Registry.Spawn(CurrentWorld, Request.ClassName, Request.Location, Request.Rotation);
    if (!Actor || Actor->GetWorld() != CurrentWorld || Actor->HasBegunPlay()) {
      if (Actor) {
        Actor->Destroy();
      }
      return EAutomationWorldMutationStatus::InvalidState;
    }
    if (!Actor->SetActorScale(Request.Scale) ||
        (Request.InstanceName &&
         !ActorManager->AssignInstanceName(*Actor, *Request.InstanceName))) {
      Actor->Destroy();
      return EAutomationWorldMutationStatus::InvalidState;
    }

    Actor->Spawned();
    return MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot) ==
                   EAutomationWorldReadStatus::Success
               ? EAutomationWorldMutationStatus::Success
               : EAutomationWorldMutationStatus::InvalidState;
  };

  FAutomationDestroyActorProvider DestroyActorProvider = [](FActorId ActorId) {
    World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return EAutomationWorldMutationStatus::WorldNotAvailable;
    }

    FActorManager* ActorManager = CurrentWorld->GetActorManager();
    if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
      return EAutomationWorldMutationStatus::InvalidState;
    }

    AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
    if (!Actor) {
      return EAutomationWorldMutationStatus::ActorNotFound;
    }
    if (Actor->IsPendingDestroy()) {
      return EAutomationWorldMutationStatus::ActorPendingDestroy;
    }

    Actor->Destroy();
    return EAutomationWorldMutationStatus::Success;
  };

  FAutomationPatchActorTransformProvider PatchActorTransformProvider =
      [](FActorId ActorId,
         const FAutomationTransformPatch& Patch,
         FAutomationActorSnapshot& OutSnapshot) {
        World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
        if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
          return EAutomationWorldMutationStatus::WorldNotAvailable;
        }

        FActorManager* ActorManager = CurrentWorld->GetActorManager();
        if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
          return EAutomationWorldMutationStatus::InvalidState;
        }

        AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
        if (!Actor) {
          return EAutomationWorldMutationStatus::ActorNotFound;
        }
        if (Actor->IsPendingDestroy()) {
          return EAutomationWorldMutationStatus::ActorPendingDestroy;
        }

        if ((Patch.Location && !Actor->SetActorLocation(*Patch.Location)) ||
            (Patch.Rotation && !Actor->SetActorRotation(*Patch.Rotation)) ||
            (Patch.Scale && !Actor->SetActorScale(*Patch.Scale))) {
          return EAutomationWorldMutationStatus::InvalidState;
        }
        return MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot) ==
                       EAutomationWorldReadStatus::Success
                   ? EAutomationWorldMutationStatus::Success
                   : EAutomationWorldMutationStatus::InvalidState;
      };

  FAutomationActorResolver ActorResolver = [](FActorId ActorId, AActor*& OutActor) {
    OutActor = nullptr;
    World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return EAutomationActorResolveStatus::WorldNotAvailable;
    }

    FActorManager* ActorManager = CurrentWorld->GetActorManager();
    if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
      return EAutomationActorResolveStatus::InvalidState;
    }

    AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
    if (!Actor || Actor->GetWorld() != CurrentWorld) {
      return EAutomationActorResolveStatus::ActorNotFound;
    }
    if (Actor->IsPendingDestroy()) {
      return EAutomationActorResolveStatus::ActorPendingDestroy;
    }

    OutActor = Actor;
    return EAutomationActorResolveStatus::Success;
  };

  AutomationApiController = std::make_unique<FAutomationApiController>(
      *AutomationCommandQueue,
      Config,
      std::move(StateProvider),
      std::move(ActorListProvider),
      std::move(ActorProvider),
      std::move(SpawnActorProvider),
      std::move(DestroyActorProvider),
      std::move(PatchActorTransformProvider),
      AutomationMethodRegistry.get(),
      std::move(ActorResolver),
      AutomationSystemCommandRegistry.get()
  );
  AutomationHttpServer = std::make_unique<FAutomationHttpServer>(Config, *AutomationApiController);
  if (!AutomationHttpServer->Start()) {
    M_LOG("Automation server startup failed; the engine will continue without Automation.");
    AutomationHttpServer.reset();
    AutomationApiController.reset();
    AutomationSystemCommandRegistry.reset();
    AutomationMethodRegistry.reset();
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

  if (bPaused) return true;

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
