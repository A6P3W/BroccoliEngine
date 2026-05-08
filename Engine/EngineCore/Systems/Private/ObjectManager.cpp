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
	for (auto& object : m_Actors) {
		object->Update(DeltaTime);
	}
	std::erase_if(m_Actors, [this](const std::unique_ptr<AActor>& obj) {
		auto* gameObject = obj.get();
		if (gameObject && gameObject->IsPendingDestroy()) {
			m_Actors.erase(
				std::remove(m_Actors.begin(), m_Actors.end(), obj),
				m_Actors.end());
			return true;
		}
		return false;
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
