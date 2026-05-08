#pragma once
#include <memory>
#include <functional>
#include "GameModeBase.h"

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
		m_PendingSceneFactory = []()->std::unique_ptr<AGameModeBase> {
			return std::make_unique<T>();
			};
	}

	AGameModeBase* GetCurrentScene() { return m_CurrentScene.get(); }

	void ProcessSceneChanges();
private:
	std::unique_ptr<AGameModeBase> m_CurrentScene;
	std::function<std::unique_ptr<AGameModeBase>()> m_PendingSceneFactory;

};

