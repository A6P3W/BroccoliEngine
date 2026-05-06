#include "Application.h"
#include "DxLib.h"
#include "Objects/SampleA.h"
#include "RenderSystem.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "ObjectManager.h"
#include "SceneManager.h"
#include "GameModeBase.h"
#include "Objects/DefaultScene.h"
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
    auto& IMI = InputManager::GetInstance();
    while (ProcessMessage() == 0 && !IMI.GetKeyPressing(KEY_INPUT_ESCAPE)|| IMI.GetKeyPressing(KEY_INPUT_LSHIFT)) {

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
    return true;
}
bool Application::Update(float DeltaTime)
{
	InputManager::GetInstance().Update();
	SceneManager::GetInstance().ProcessSceneChanges();
    ObjectManager::GetInstance().Update(DeltaTime);
    if (AGameModeBase* m_CurrentScene = SceneManager::GetInstance().GetCurrentScene()) {
		m_CurrentScene->OnUpdate(DeltaTime);

    }

	return true;
}

bool Application::Draw()
{
	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();

    ObjectManager::GetInstance().Draw();
	RenderSystem::GetInstance().Draw();
	ScreenFlip();
	return true;
}

