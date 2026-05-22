#pragma once
#include "CollisionComponent.h"
#include <vector>
#include <unordered_map>
#include "CircleCollisionComponent.h"
#include "RectangleCollisionComponent.h"
#include "LineCollisionComponent.h"
#include "MovementComponent.h"
#include <Actor.h>
class CollisionSystem
{

public:
	static CollisionSystem& GetInstance();
	void RegisterCollision(MCollisionComponent* component);
	void UnRegisterCollision(MCollisionComponent* component);
	void CheckCollisions();
private:
	void CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b);
	void CircleAndRectangle(MCircleCollisionComponent* circle, MRectangleCollisionComponent* rect);
	void RectangleAndRectangle(MRectangleCollisionComponent* a, MRectangleCollisionComponent* b);
	void LineAndCircle(MLineCollisionComponent* line, MCircleCollisionComponent* circle);
	void LineAndRectangle(MLineCollisionComponent* line, MRectangleCollisionComponent* rect);
	void LineAndLine(MLineCollisionComponent* a, MLineCollisionComponent* b);

	static void CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal);

	void CollisionResolution(AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth);

	std::vector<MCollisionComponent*> m_CollisionComponents;
};

