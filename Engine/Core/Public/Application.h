#pragma once
#include <memory>
#include <vector>
class AGameModeBase;
class Application
{
public:
	Application();
	bool Run();
private:
	bool Update(float DeltaTime);
	bool Draw();
	float m_DeltaTime = 0.0f;
	AGameModeBase* m_CurrentScene = nullptr;
	bool m_posed = false;
};

