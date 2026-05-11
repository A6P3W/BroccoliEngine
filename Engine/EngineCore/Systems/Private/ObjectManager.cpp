#include "ObjectManager.h"
#include <algorithm>

ObjectManager& ObjectManager::GetInstance()
{
	static ObjectManager Instance;
	return Instance;
}

ObjectManager::ObjectManager()
{
}

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
			object->Update(DeltaTime);
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
