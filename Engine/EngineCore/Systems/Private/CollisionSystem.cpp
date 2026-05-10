#include "CollisionSystem.h"
#include "Actor.h"
#include "CircleCollisionComponent.h"
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
	float Ax = locA.X, Ay = locA.Y, Bx = locB.X, By = locB.Y;
	auto aActor = a->GetOwner();
	auto bActor = b->GetOwner();
	if ((Bx - Ax) * (Bx - Ax) + (By - Ay) * (By - Ay) <= (radA + radB) * (radA + radB)) {
		a->UpdateOverlapState(bActor, true);
		b->UpdateOverlapState(aActor, true);
	}
	else {
		a->UpdateOverlapState(bActor, false);
		b->UpdateOverlapState(aActor, false);
	}
}
