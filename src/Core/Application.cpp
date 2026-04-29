#include "Core/Application.h"
#include "DxLib.h"
#include "Objects/SampleA.h"
#include "Objects/OCameraObject.h"
#include "Systems/RenderSystem.h"
#include "Objects/OGridLine.h"
Application::Application()
{
	auto sample_a = std::make_unique<SampleA>(50.0f, 2.0f);
	auto camera = std::make_unique<OCameraObject>();
	camera->SetTarget(sample_a.get());
	camera->SetCameraView();
	m_GameObjects.push_back(std::move(sample_a));
	m_GameObjects.push_back(std::move(camera));

	m_GameObjects.push_back(std::make_unique<OGridLine>(50));

}
bool Application::Run()
{
	SetWaitVSyncFlag(FALSE);
	const LONGLONG TargetFrameTime = 1000000 / 60;
	LONGLONG NextFrameTime = GetNowHiPerformanceCount();
	while (ProcessMessage() == 0) {
		Update();
		Draw();
		while (GetNowHiPerformanceCount() < NextFrameTime) {
			Sleep(0);
		}
		NextFrameTime += TargetFrameTime;
	}
	return true;
}
bool Application::Update()
{
	for (auto& object : m_GameObjects) {
		object->Update();
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

