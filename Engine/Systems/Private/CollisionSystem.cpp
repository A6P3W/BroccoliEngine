#include "CollisionSystem.h"

void CollisionSystem::RegisterCollision(MCollisionComponent* component)
{
	m_CollisionComponents.push_back(component);
}

void CollisionSystem::UnRegisterCollision(MCollisionComponent* component)
{
	std::erase(m_CollisionComponents, component);
}

void CollisionSystem::CheckCollisions()
{
	int size = m_CollisionComponents.size();
	for (int i = 0; i < size; i++) {
		for (int j = i + 1; j < size; j++) {
			auto* a = m_CollisionComponents[i];
			auto* b = m_CollisionComponents[j];
			if (a->owner() == b->owner()) { continue; }
		}
}
