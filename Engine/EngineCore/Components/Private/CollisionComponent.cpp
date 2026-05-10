#include "CollisionComponent.h"
#include <CollisionSystem.h>
MCollisionComponent::MCollisionComponent()
{
	CollisionSystem::GetInstance().RegisterCollision(this);

}

MCollisionComponent::~MCollisionComponent()
{
	CollisionSystem::GetInstance().UnRegisterCollision(this);
}

