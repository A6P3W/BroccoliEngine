#include "Application.h"
#include "DxLib.h"
#include "SampleA.h"
#include "Player.h"
Application::Application()
{
	for (int i = 0;i < 2;i++) {
		m_GameObjects.push_back(std::make_unique<SampleA>(float(i*50), 2.0f));
	}

}
bool Application::Run()
{
	
	while (ProcessMessage() == 0) {
		Update();
		Draw();
	}
	return true;
}
bool Application::Update()
{
	m_GameObjects.push_back(std::make_unique<SampleA>(100, 100.0f));
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
	ScreenFlip();
	return true;
}

