#include "ObjectManager.h"
#include <algorithm>
#include "Actor.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager() = default;

void ObjectManager::Update(float DeltaTime)
{
	// 更新対象のポインタをコピーしてループを回す
	std::vector<AActor*> tempActors;
	tempActors.reserve(m_Actors.size());
	for (auto& object : m_Actors) {
		tempActors.push_back(object.get());
	}
	for (auto* object : tempActors) {
		if (object && !object->IsPendingDestroy()) {
			if (m_World->IsSimulating() || object->IsEditorActor() || object->CanUpdateAnytime()) {
				object->Update(DeltaTime);
			}
		}
	}

	std::erase_if(m_Actors, [](const std::unique_ptr<AActor>& obj) {
		return !obj || obj->IsPendingDestroy();
		});
}

void ObjectManager::Draw()
{
	for (auto& object : m_Actors) {
		object->Draw();
	}
}

void ObjectManager::ClearAllObjects()
{
	m_Actors.clear();

}
