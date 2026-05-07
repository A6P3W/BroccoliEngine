#pragma once
#include "CollisionComponent.h"
#include <vector>
class CollisionSystem
{
	static CollisionSystem& GetInstance() {
		static CollisionSystem instance;
		return instance;
	}
public:
	void RegisterCollision(MCollisionComponent* component);
	void UnRegisterCollision(MCollisionComponent* component);
private:
	std::vector<MCollisionComponent*> m_CollisionComponents;
};

