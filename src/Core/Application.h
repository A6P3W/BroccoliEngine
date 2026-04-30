#pragma once
#include <memory>
#include "Core/GameObject.h"
#include <vector>
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update(float DeltaTime);
	bool Draw();
	std::vector< std::unique_ptr<AGameObject>> m_GameObjects;
	float m_DeltaTime = 0.0f;
};

