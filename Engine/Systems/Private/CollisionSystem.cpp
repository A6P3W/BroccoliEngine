#include "CollisionSystem.h"

void CollisionSystem::RegisterCollision(MCollisionComponent* component)
{
	m_CollisionComponents.push_back(component);
}

void CollisionSystem::UnRegisterCollision(MCollisionComponent* component)
{
	std::erase(m_CollisionComponents, component);
}
