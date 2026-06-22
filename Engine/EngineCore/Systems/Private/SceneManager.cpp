#include "SceneManager.h"
#include "ObjectManager.h"
#include "GridLine.h"
#include "RenderSystem.h"
#include "CameraComponent.h"
#include "Actor.h"
#include "CollisionSystem.h"
#include "EngineDefine.h"
#include"GameModeBase.h"
void SceneManager::ProcessSceneChanges()
{
	if (PendingSceneFactory) {
		RenderSystem::GetInstance().SetCameraView(nullptr);

		CurrentScene = PendingSceneFactory();

		PendingSceneFactory = nullptr;

		if (IsDebug) {
			auto Grid = CurrentScene->SpawnActor<AGridLine>();
		}
	}
}
