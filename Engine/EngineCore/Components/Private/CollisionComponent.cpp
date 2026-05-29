#include "CollisionComponent.h"
#include <CollisionSystem.h>
MCollisionComponent::MCollisionComponent()
{

}

MCollisionComponent::~MCollisionComponent()
{
	if (CollisionSystem::IsAlive() && !IsPendingDestroy()) {
		CollisionSystem::GetInstance().UnRegisterCollision(this);
	}
}
void MCollisionComponent::MarkCheckedThisFrame(AActor* OtherActor) {
    m_CheckedThisFrame.insert(OtherActor);
}

void MCollisionComponent::FlushOverlapState() {
    std::vector<AActor*> toRemove;
    for (AActor* actor : m_OverlappingActors) {
        if (m_IntersectingThisFrame.find(actor) == m_IntersectingThisFrame.end()) {
            toRemove.push_back(actor);
        }
    }
    for (AActor* actor : toRemove) {
        m_OverlappingActors.erase(actor);
        GetOwner()->EndOverlap(actor);
    }
    m_CheckedThisFrame.clear();
    m_IntersectingThisFrame.clear();
}
void MCollisionComponent::OnComponentDestroy()
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
