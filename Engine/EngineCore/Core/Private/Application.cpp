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
#include "GamePadDevice.h"
#include "ObjectManager.h"
#include "SceneManager.h"
#include "GameModeBase.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "EngineDefine.h"
#include "EditorMode.h"
#include "HttpManager.h"
extern void SetupGame();

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ImGuiHookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return 1;
	}
	return 0;
}

Application::Application()
{}

Application::~Application() {
	if (OffscreenBuffer != -1) {
		DeleteGraph(OffscreenBuffer);
	}
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Application::InitOffscreenBuffer()
{
	if (OffscreenBuffer != -1) {
		DeleteGraph(OffscreenBuffer);
	}
	OffscreenBuffer = MakeScreen(VirtualWidth, VirtualHeight, true);
}

void Application::SetWindowResolution(int width, int height)
{
	SetWindowSize(width, height);
}
static bool ShouldQuitGame = false;
void Application::QuitGame()
{
	ShouldQuitGame = true;
}

bool Application::Run()
{
	SetGraphMode(1920, 1080, 32);

	SetUseDirect3D11(true);

	ChangeWindowMode(true);

	bool bFitScreen = !IsEditor;
	bool bFullScreen = !IsRelease;

	SetWindowSizeChangeEnableFlag(true, bFitScreen);
	ChangeWindowMode(bFullScreen);

	DxLib_Init();


	SetWaitVSyncFlag(false);
	const LONGLONG TargetFrameTime = 1000000 / 60;
	LONGLONG LastTime = GetNowHiPerformanceCount();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(GetMainWindowHandle());
	ImGui_ImplDX11_Init(
		reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device())),
		reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()))
	);

	SetHookWinProc(ImGuiHookProc);

	InitOffscreenBuffer();

	if (IsEditor) {
		SceneManager::GetInstance().OpenScene<EditorMode>();
	}
	else {
		SetupGame();
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

	while (ProcessMessage() == 0 && !ShouldQuitGame) {
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
		if (DeltaTime > 0.1f) DeltaTime = 0.1f;

		Update(DeltaTime);
		Draw();
	}
	return true;
}
bool Application::Update(float DeltaTime)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	int screenW, screenH;
	GetDrawScreenSize(&screenW, &screenH);
	io.DisplaySize = ImVec2((float)screenW, (float)screenH);

	int mx, my;
	GetMousePoint(&mx, &my);
	io.MousePos = ImVec2((float)mx, (float)my);

	ImGui::NewFrame();
	InputManager::GetInstance().Update();
	HttpManager::GetInstance().Update();

	if (bPosed) return true;

	SceneManager::GetInstance().ProcessSceneChanges();
	if (World* currentScene = SceneManager::GetInstance().GetCurrentScene()) {
		currentScene->Update(DeltaTime);
	}
	return true;
}
bool Application::Draw()
{
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
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ScreenFlip();
	return true;
}
