#include "ObjectManager.h"

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
	for (auto& object : m_GameObjects) {
		object->Update(DeltaTime);
	}
}

void ObjectManager::Draw()
{
	for (auto& object : m_GameObjects) {
		object->Draw();
	}
}
