#pragma once
#include <memory>
#include "Enemy.h"
#include "Player.h"
#include"GameObject.h"
#include <vector>
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update();
	bool Draw();
	std::vector< std::unique_ptr<GameObject>> m_GameObjects;
};

