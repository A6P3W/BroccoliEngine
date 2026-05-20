#include "CollisionSystem.h"
#include "Actor.h"
#include "CircleCollisionComponent.h"
#include "MovementComponent.h"
CollisionSystem& CollisionSystem::GetInstance()
{
	static CollisionSystem instance;
	return instance;
}
void CollisionSystem::RegisterCollision(MCollisionComponent* component)
{
	m_CollisionComponents.push_back(component);
}

void CollisionSystem::UnRegisterCollision(MCollisionComponent* component)
{
	auto it = std::find(m_CollisionComponents.begin(), m_CollisionComponents.end(), component);
	if (it != m_CollisionComponents.end()) {
		m_CollisionComponents.erase(it);
	}
}

void CollisionSystem::CheckCollisions()
{
	for (size_t i = 0; i < m_CollisionComponents.size(); ++i) {
		for (size_t j = i + 1; j < m_CollisionComponents.size(); ++j) {
			auto* A = m_CollisionComponents[i];
			auto* B = m_CollisionComponents[j];
			if (A->GetOwner() == B->GetOwner()) continue;
			if (A->GetShapeType() == ECollisionShape::Circle && B->GetShapeType() == ECollisionShape::Circle) {
				CircleAndCircle(static_cast<MCircleCollisionComponent*>(A), static_cast<MCircleCollisionComponent*>(B));
			}
		}
	}
}

void CollisionSystem::CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b)
{
	FVector2D locA = a->GetWorldLocation(), locB = b->GetWorldLocation();
	float radA = a->GetRadius(), radB = b->GetRadius();
	float dx = locB.X - locA.X;
	float dy = locB.Y - locA.Y;
	float distanceSquared = dx * dx + dy * dy;
	float minDistance = radA + radB;

	auto aActor = a->GetOwner();
	auto bActor = b->GetOwner();

	if (distanceSquared <= minDistance * minDistance) {
		a->UpdateOverlapState(bActor, true);
		b->UpdateOverlapState(aActor, true);

		if (a->GetCollisionType() == ECollisionType::Block && b->GetCollisionType() == ECollisionType::Block) {
			float distance = std::sqrt(distanceSquared);

			FVector2D normal = { dx / distance, dy / distance };

			float overlapDepth = minDistance - distance;


			auto* moveA = aActor->GetComponents<MMovementComponent>()[0];
			auto* moveB = bActor->GetComponents<MMovementComponent>()[0];

			if (moveA && moveB) {
				moveA->SetWorldForce({ 0,0 });
				moveB->SetWorldForce({ 0,0 });

				aActor->AddActorWorldOffset(normal * (-overlapDepth * 0.5f));
				bActor->AddActorWorldOffset(normal * (overlapDepth * 0.5f));
			}
			else if (moveA) {
				aActor->AddActorWorldOffset(normal * (-overlapDepth));
			}
			else if (moveB) {
				bActor->AddActorWorldOffset(normal * overlapDepth);
			}
		}
	}
	else {
		a->UpdateOverlapState(bActor, false);
		b->UpdateOverlapState(aActor, false);
	}

}
