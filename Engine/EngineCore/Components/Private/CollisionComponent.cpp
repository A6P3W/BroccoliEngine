#include "CollisionComponent.h"
#include <CollisionSystem.h>
MCollisionComponent::MCollisionComponent()
{

}

MCollisionComponent::~MCollisionComponent()
{
	if (CollisionSystem::IsAlive()) {
		CollisionSystem::GetInstance().UnRegisterCollision(this);
	}
}

void MCollisionComponent::SetStatic(bool IsStatic)
{
	if (bIsStatic == IsStatic) {
		return;
	}
	bIsStatic = IsStatic;
	CollisionSystem::GetInstance().RebuildStaticCollisionMap();
}

