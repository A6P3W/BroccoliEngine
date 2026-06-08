#include "CollisionComponent.h"
#include <CollisionSystem.h>
#include "World.h"

MCollisionComponent::MCollisionComponent()
{

}

MCollisionComponent::~MCollisionComponent()
{
	if(!GetOwner()->GetWorld()->IsTrendingDown()){
	UnRegisterComponent();
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
void MCollisionComponent::RegisterComponent()
{
	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
		GetOwner()->GetWorld()->GetCollisionSystem()->RegisterCollision(this);
	}
}

void MCollisionComponent::UnRegisterComponent()
{

	if (GetOwner() && GetOwner()->GetWorld()) {
		if (auto CS = GetOwner()->GetWorld()->GetCollisionSystem()) {
			CS->UnRegisterCollision(this);
		}
	}

}
void MCollisionComponent::OnComponentDestroy()
{}

void MCollisionComponent::SetStatic(bool IsStatic)
{
	if (bIsStatic == IsStatic) {
		return;
	}
	bIsStatic = IsStatic;
	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
		GetOwner()->GetWorld()->GetCollisionSystem()->RebuildStaticCollisionMap();
	}
}
