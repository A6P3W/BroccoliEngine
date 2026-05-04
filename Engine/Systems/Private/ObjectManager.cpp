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
	for (auto& object : m_UpdateAbleObject) {
		object->Update(DeltaTime);
	}
    std::erase_if(m_UpdateAbleObject, [this](const std::unique_ptr<MUpdateableObject>& obj) {
		auto* gameObject = dynamic_cast<AGameObject*>(obj.get());
		if (gameObject && gameObject->IsPendingDestroy()) {
			m_RenderAbleObject.erase(
				std::remove(m_RenderAbleObject.begin(), m_RenderAbleObject.end(), gameObject),
				m_RenderAbleObject.end());
			return true;
		}
		return false;
		});
}

void ObjectManager::Draw()
{
   for (auto* object : m_RenderAbleObject) {
		object->Draw();
	}
}
