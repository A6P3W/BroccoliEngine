#pragma once
#include <memory>
#include "Core/Public/GameObject.h"
#include <vector>
#include "GameMain.h"
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update(float DeltaTime);
	bool Draw();
	float m_DeltaTime = 0.0f;
	std::unique_ptr<GameMain> GameMainInstance = std::make_unique<GameMain>();
};

