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
	if (m_PendingSceneFactory) {
		RenderSystem::GetInstance().SetCameraView(nullptr);

		m_CurrentScene = m_PendingSceneFactory();

		m_PendingSceneFactory = nullptr;

		if (IsDebug) {
			auto Grid = m_CurrentScene->SpawnActor<AGridLine>();
		}
	}
}
