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
	CollisionSystem();
	~CollisionSystem();

	static void Initialize();
	static void Terminate();

	static bool IsAlive();
	void RegisterCollision(MCollisionComponent* component);
	void UnRegisterCollision(MCollisionComponent* component);
	void RebuildStaticCollisionMap();
	void BeginSceneTransition();
	void EndSceneTransition();

	void UpdateCollisionMap();

	void CheckCollisions();

	float GetCollisionCellSize() { return m_CollisionCellSize; }
private:
	void CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b);
	void CircleAndRectangle(MCircleCollisionComponent* circle, MRectangleCollisionComponent* rect);
	void RectangleAndRectangle(MRectangleCollisionComponent* a, MRectangleCollisionComponent* b);
	void LineAndCircle(MLineCollisionComponent* line, MCircleCollisionComponent* circle);
	void LineAndRectangle(MLineCollisionComponent* line, MRectangleCollisionComponent* rect);
	void LineAndLine(MLineCollisionComponent* a, MLineCollisionComponent* b);

	static void CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal);

	void CollisionResolution(AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth);

	void CheckCollisionPair(MCollisionComponent* A, MCollisionComponent* B);

	static std::unique_ptr<CollisionSystem> s_Instance;

	std::vector<MCollisionComponent*> m_CollisionComponents;
	std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash> m_StaticCollisionMap;
	std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash> m_DynamicCollisionMap;
	float m_CollisionCellSize = 100;
	std::uint64_t m_FrameId = 0;
	bool m_DeferStaticRebuild = false;
	bool m_PendingStaticRebuild = false;

	std::vector<MCollisionComponent*> m_PendingStaticRegistrations;
	void RegisterToStaticMap(MCollisionComponent* component);
};

