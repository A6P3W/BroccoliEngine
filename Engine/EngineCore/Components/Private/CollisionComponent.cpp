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
	CheckedThisFrame.insert(OtherActor);
}

void MCollisionComponent::FlushOverlapState() {
	std::vector<AActor*> toRemove;
	for (AActor* actor : OverlappingActors) {
		if (IntersectingThisFrame.find(actor) == IntersectingThisFrame.end()) {
			toRemove.push_back(actor);
		}
	}
	for (AActor* actor : toRemove) {
		OverlappingActors.erase(actor);
		GetOwner()->EndOverlap(actor);
	}
	CheckedThisFrame.clear();
	IntersectingThisFrame.clear();
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
