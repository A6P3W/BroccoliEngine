#include "Core/Application.h"
#include "DxLib.h"
#include "Objects/SampleA.h"
#include <Systems/RenderSystem.h>
#include "Objects/GridLine.h"
#include <Systems/InputManager.h>
#include <Systems/InputMapper.h>
#include <Systems/ObjectManager.h>
Application::Application()
{
    auto Grid = ObjectManager::GetInstance().SpawnObject<AGridLine>();
    Grid->SetLineWidth(100);
}
bool Application::Run()
{
    SetWaitVSyncFlag(FALSE);

    const LONGLONG TargetFrameTime = 1000000 / 60;
    LONGLONG LastTime = GetNowHiPerformanceCount();

	GameMainInstance->BeginPlay();
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
    ObjectManager::GetInstance().Update(DeltaTime);
	GameMainInstance->Update(DeltaTime);

	return true;
}

bool Application::Draw()
{
	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();
	GameMainInstance->Draw();
    ObjectManager::GetInstance().Draw();
	RenderSystem::GetInstance().Draw();
	ScreenFlip();
	return true;
}

