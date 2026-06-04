#pragma once
#include <memory>
#include <functional>
#include "GameModeBase.h"
#include "World.h"
class SceneManager
{
public:
	static SceneManager& GetInstance()
	{
		static SceneManager instance;
		return instance;
	}
	template<class T>
	void OpenScene()
	{
		m_PendingSceneFactory = []()->std::unique_ptr<World> {
			auto newWorld = std::make_unique<World>();
			newWorld->SpawnGameMode<T>();
			return newWorld;
			};
	}

	World* GetCurrentScene() { return m_CurrentScene.get(); }

	void ProcessSceneChanges();
private:
	std::unique_ptr<World> m_CurrentScene;
	std::function<std::unique_ptr<World>()> m_PendingSceneFactory;

};

