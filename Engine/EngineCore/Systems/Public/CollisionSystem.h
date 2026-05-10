#pragma once
#include "CollisionComponent.h"
#include <vector>
#include <unordered_map>
#include "CircleCollisionComponent.h"

class CollisionSystem
{

public:
	static CollisionSystem& GetInstance();
	void RegisterCollision(MCollisionComponent* component);
	void UnRegisterCollision(MCollisionComponent* component);
	void CheckCollisions();
private:
	void CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b);
	std::vector<MCollisionComponent*> m_CollisionComponents;
};

