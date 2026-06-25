#include "SceneManager.h"
#include "ObjectManager.h"
#include "GridLine.h"
#include "RenderSystem.h"
#include "CameraComponent.h"
#include "Actor.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include"GameModeBase.h"
#include "ReplicationSystem.h"

bool SceneManager::OpenSceneById(FNetworkSceneId SceneId)
{
	return OpenSceneById(SceneId, GetCurrentNetMode());
}

bool SceneManager::OpenSceneById(FNetworkSceneId SceneId, ENetMode NetMode)
{
	auto it = RegisteredSceneFactories.find(SceneId);
	if (it == RegisteredSceneFactories.end()) {
		return false;
	}

	QueueSceneFactory(it->second, NetMode, SceneId);
	return true;
}

bool SceneManager::IsSceneRegistered(FNetworkSceneId SceneId) const
{
	return RegisteredSceneFactories.find(SceneId) != RegisteredSceneFactories.end();
}

ENetMode SceneManager::GetCurrentNetMode() const
{
	if (!CurrentScene) {
		return ENetMode::Standalone;
	}

	return CurrentScene->GetNetMode();
}

void SceneManager::QueueSceneFactory(RegisteredSceneFactory Factory, ENetMode NetMode, FNetworkSceneId SceneId)
{
	if (!Factory) {
		return;
	}

	PendingSceneFactory = [Factory = std::move(Factory), NetMode]()->std::unique_ptr<World> {
		return Factory(NetMode);
	};
	PendingSceneId = SceneId;
}

void SceneManager::ProcessSceneChanges()
{
	if (PendingSceneFactory) {
		RenderSystem::GetInstance().SetCameraView(nullptr);

		CurrentScene.reset();
		CurrentScene = PendingSceneFactory();
		CurrentSceneId = PendingSceneId;

		PendingSceneFactory = nullptr;
		PendingSceneId = 0;

		if (CurrentScene && IsDebug) {
			auto Grid = CurrentScene->SpawnActor<AGridLine>();
		}

		if (CurrentScene && CurrentScene->GetReplicationSystem()) {
			CurrentScene->GetReplicationSystem()->NotifySceneLoaded();
		}
	}
}
