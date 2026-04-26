#include "Application.h"
#include "DxLib.h"
#include "Enemy.h"
#include "Player.h"
Application::Application()
{
	for (int i = 0;i < 2;i++) {
		m_GameObjects.push_back(std::make_unique<Enemy>(float(i*50), 2.0f));
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

