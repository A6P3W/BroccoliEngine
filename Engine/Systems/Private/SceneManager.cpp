#include "SceneManager.h"
#include "ObjectManager.h"
#include "GridLine.h"
#include "RenderSystem.h"
#include "CameraComponent.h"
#include "Actor.h"
void SceneManager::ProcessSceneChanges()
{
	if (m_PendingSceneFactory) {
		RenderSystem::GetInstance().SetCameraView(nullptr);
		ObjectManager::GetInstance().ClearAllObjects();

		m_CurrentScene = m_PendingSceneFactory();
		m_PendingSceneFactory = nullptr;
		auto Grid = ObjectManager::GetInstance().SpawnObject<AGridLine>();
		Grid->SetLineWidth(100);


	}
}
