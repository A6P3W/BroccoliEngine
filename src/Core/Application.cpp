#include "Core/Application.h"
#include "DxLib.h"
#include "Objects/SampleA.h"
#include "Objects/OCameraObject.h"
#include "Systems/RenderSystem.h"
#include "Objects/OGridLine.h"
#include "Systems/InputManager.h"
#include "Systems/InputMapper.h"
#include "Objects/Player.h"
Application::Application()
{
	m_GameObjects.push_back(std::make_unique<OGridLine>(50));


	auto sample_a = std::make_unique<SampleA>(50.0f, 2.0f);
	auto camera = std::make_unique<OCameraObject>();
	camera->SetTarget(sample_a.get());
	camera->SetCameraView();
	m_GameObjects.push_back(std::move(sample_a));
	
	auto camera2 = std::make_unique<OCameraObject>();
	auto player = std::make_unique<Player>(-50.0f, -50.0f);
	camera2->SetTarget(player.get());
	camera2->SetCameraView();
	m_GameObjects.push_back(std::move(player));
	m_GameObjects.push_back(std::move(camera2));
}
bool Application::Run()
{
    SetWaitVSyncFlag(FALSE);

    const LONGLONG TargetFrameTime = 1000000 / 60;
    LONGLONG LastTime = GetNowHiPerformanceCount();

    while (ProcessMessage() == 0 && !InputMapper::GetInstance().GetKeyPressStart(E_INPUT_ACTION::CANCEL)) {

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
	for (auto& object : m_GameObjects) {
		object->Update(DeltaTime);
	}
	return true;
}

bool Application::Draw()
{
	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();
	for (auto& object : m_GameObjects) {
		object->Draw();
	}
	RenderSystem::GetInstance().Draw();
	ScreenFlip();
	return true;
}

