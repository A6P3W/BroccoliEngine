#include "Application.h"
#include "DxLib.h"
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
extern void SetupGame();
Application::Application()
{
}
bool Application::Run()
{
    SetWaitVSyncFlag(FALSE);
    const LONGLONG TargetFrameTime = 1000000 / 60;
    LONGLONG LastTime = GetNowHiPerformanceCount();

    SetupGame();
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
	ScreenFlip();
	return true;
}

