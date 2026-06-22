#pragma once
#include <memory>
#include <functional>
#include "GameModeBase.h"
#include "World.h"
#include "GameInstance.h"
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
		PendingSceneFactory = []()->std::unique_ptr<World> {
			auto newWorld = std::make_unique<World>();
			newWorld->SpawnGameMode<T>();
			return newWorld;
			};
	}

	World* GetCurrentScene() { return CurrentScene.get(); }

	void ProcessSceneChanges();

	template<class T>
	void SetGameInstance() { GameInstance = std::make_unique<T>(); }
	GameInstance* GetGameInstance() { return GameInstance.get(); }
private:
	std::unique_ptr<World> CurrentScene;
	std::function<std::unique_ptr<World>()> PendingSceneFactory;
	std::unique_ptr<GameInstance> GameInstance = nullptr;
};

