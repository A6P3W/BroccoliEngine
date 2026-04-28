#pragma once
#include <memory>
#include "OGameObject.h"
#include <vector>
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update();
	bool Draw();
	std::vector< std::unique_ptr<OGameObject>> m_GameObjects;
};

