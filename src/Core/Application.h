#pragma once
#include <memory>
#include "Core/OGameObject.h"
#include <vector>
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update(float DeltaTime);
	bool Draw();
	std::vector< std::unique_ptr<OGameObject>> m_GameObjects;
	float m_DeltaTime = 0.0f;
};

