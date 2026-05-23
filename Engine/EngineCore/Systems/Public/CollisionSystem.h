#pragma once
#include "CollisionComponent.h"
#include <vector>
#include <unordered_map>
#include "CircleCollisionComponent.h"
#include "RectangleCollisionComponent.h"
#include "LineCollisionComponent.h"
#include "MovementComponent.h"
#include <Actor.h>
#include <utility>
#include <map>

struct pair_hash {
	inline std::size_t operator()(const std::pair<int, int>& v) const {
		return v.first * 31 + v.second;
	}
};
class CollisionSystem
{

public:
	static CollisionSystem& GetInstance();
	void RegisterCollision(MCollisionComponent* component);
	void UnRegisterCollision(MCollisionComponent* component);

	void UpdateCollisionMap();

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


	void CheckCell(std::vector<MCollisionComponent*> Cell);
	void CheckCellPair(std::vector<MCollisionComponent*> CellA, std::vector<MCollisionComponent*> CellB);
	void CheckCollisionPair(MCollisionComponent* A,MCollisionComponent* B);


	std::vector<MCollisionComponent*> m_CollisionComponents;

	std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash> CollisionMap;
	float m_CollisionMapSize = 30;
};

