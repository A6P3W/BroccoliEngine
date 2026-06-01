#include "Application.h"
#include "DxLib.h"

#include <imgui.h>
#include <imgui_impl/imgui_impl_win32.h>
#include <imgui_impl/imgui_impl_dx11.h>

#include "RenderSystem.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "KeyboardDevice.h"
#include "MouseDevice.h"
#include "ObjectManager.h"
#include "SceneManager.h"
#include "GameModeBase.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "EngineDefine.h"
#include "EditorMode.h"
extern void SetupGame();

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ImGuiHookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return 1;
    }
    return 0; 
}

Application::Application()
{
}
Application::~Application() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
bool Application::Run()
{
    SetWaitVSyncFlag(FALSE);
    const LONGLONG TargetFrameTime = 1000000 / 60;
    LONGLONG LastTime = GetNowHiPerformanceCount();

    // --- ImGuiの初期化 ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // DxLibからウィンドウハンドルとDX11デバイスを取得してImGuiに渡す
    ImGui_ImplWin32_Init(GetMainWindowHandle());
    ImGui_ImplDX11_Init(
        reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device())),
        reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()))
    );
    // マウス入力等をImGuiに流すためにフックを設定
    SetHookWinProc(ImGuiHookProc);
    if (IsDebug) {
        SceneManager::GetInstance().OpenScene<EditorMode>();
    }
    else {
        SetupGame();
    }
    auto& IM = InputManager::GetInstance();
    IM.AddDevice(std::make_unique<KeyboardDevice>());
    IM.AddDevice(std::make_unique<MouseDevice>());
    while (ProcessMessage() == 0) {

        LONGLONG CurrentTime = GetNowHiPerformanceCount();
        LONGLONG ElapsedTime = CurrentTime - LastTime;


        if (ElapsedTime < TargetFrameTime) {

            LONGLONG SleepTime = (TargetFrameTime - ElapsedTime) / 1000; 
            if (SleepTime > 0) {
                Sleep((DWORD)SleepTime);
            }
            CurrentTime = GetNowHiPerformanceCount();
            ElapsedTime = CurrentTime - LastTime;
        }

        m_DeltaTime = static_cast<float>(ElapsedTime) / 1000000.0f;
        LastTime = CurrentTime;

        if (m_DeltaTime > 0.1f) m_DeltaTime = 0.1f;


        Update(m_DeltaTime);
        Draw();
    }
    CollisionSystem::GetInstance().BeginSceneTransition();
    ObjectManager::GetInstance().ClearAllObjects();
    CollisionSystem::GetInstance().EndSceneTransition();
    return true;
}
bool Application::Update(float DeltaTime)
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //ImGui::ShowDemoWindow();
	InputManager::GetInstance().Update();

    if (m_posed) {
        return true;
    }
	SceneManager::GetInstance().ProcessSceneChanges();
    ObjectManager::GetInstance().Update(DeltaTime);
    TimerManager::GetInstance().Update(DeltaTime);
    if (AGameModeBase* m_CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
        m_CurrentScene->Update(DeltaTime);

    }
    CollisionSystem::GetInstance().CheckCollisions();
	return true;
}

bool Application::Draw()
{
  SetDrawScreen(DX_SCREEN_BACK);
    ClearDrawScreen();

    if (AGameModeBase* m_CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
        m_CurrentScene->Draw();
    }

    ObjectManager::GetInstance().Draw();
    RenderSystem::GetInstance().Draw();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ScreenFlip();
	return true;
}

