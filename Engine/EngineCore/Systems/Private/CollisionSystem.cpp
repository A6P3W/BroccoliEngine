#include "CollisionSystem.h"
#include "Actor.h"
#include "CircleCollisionComponent.h"
#include "RectangleCollisionComponent.h"
#include "LineCollisionComponent.h"
#include "MovementComponent.h"
#include <atomic>
#include <algorithm>
namespace {
	std::atomic<bool> g_CollisionSystemAlive{ false };
}
CollisionSystem::CollisionSystem()
{
	g_CollisionSystemAlive.store(true, std::memory_order_release);
}

CollisionSystem::~CollisionSystem()
{
	g_CollisionSystemAlive.store(false, std::memory_order_release);
}

bool CollisionSystem::IsAlive()
{
	return g_CollisionSystemAlive.load(std::memory_order_acquire);
}
CollisionSystem& CollisionSystem::GetInstance()
{
	static CollisionSystem instance;
	return instance;
}
void CollisionSystem::RegisterCollision(MCollisionComponent* component)
{
	m_CollisionComponents.push_back(component);
	if (component->IsStatic()) {
		if (std::find(m_PendingStaticRegistrations.begin(), m_PendingStaticRegistrations.end(), component) == m_PendingStaticRegistrations.end()) {
			m_PendingStaticRegistrations.push_back(component);
		}
	}
}

void CollisionSystem::UnRegisterCollision(MCollisionComponent* component)
{
	auto it = std::find(m_CollisionComponents.begin(), m_CollisionComponents.end(), component);
	if (it != m_CollisionComponents.end()) {
		m_CollisionComponents.erase(it);
	}
	m_PendingStaticRegistrations.erase(
		std::remove(m_PendingStaticRegistrations.begin(), m_PendingStaticRegistrations.end(), component),
		m_PendingStaticRegistrations.end());
	if (component->IsStatic()) {
		if (m_DeferStaticRebuild) {
			m_PendingStaticRebuild = true;
		}
		else {
			RebuildStaticCollisionMap();
		}
	}
}

void CollisionSystem::BeginSceneTransition()
{
	m_DeferStaticRebuild = true;
}

void CollisionSystem::EndSceneTransition()
{
	m_DeferStaticRebuild = false;
	if (m_PendingStaticRebuild) {
		RebuildStaticCollisionMap();
		m_PendingStaticRebuild = false;
	}
}

void CollisionSystem::RebuildStaticCollisionMap()
{
	m_StaticCollisionMap.clear();
	for (auto* comp : m_CollisionComponents)
	{
		if (!comp->IsStatic()) continue;
		RegisterToStaticMap(comp);
		comp->SetGridClean();
	}
}

void CollisionSystem::UpdateCollisionMap()
{
	m_DynamicCollisionMap.clear();
	for (auto collision : m_CollisionComponents) {
		if (collision->IsStatic()) {
			continue;
		}
		FAABB box = collision->GetAABB();

		int minX = static_cast<int>(std::floor(box.MinX / m_CollisionCellSize));
		int maxX = static_cast<int>(std::floor(box.MaxX / m_CollisionCellSize));
		int minY = static_cast<int>(std::floor(box.MinY / m_CollisionCellSize));
		int maxY = static_cast<int>(std::floor(box.MaxY / m_CollisionCellSize));

		for (int x = minX; x <= maxX; ++x) {
			for (int y = minY; y <= maxY; ++y) {
				m_DynamicCollisionMap[{x, y}].push_back(collision);
			}
		}
		collision->SetGridClean();
	}
}

void CollisionSystem::CheckCollisions()
{
	for (auto* comp : m_PendingStaticRegistrations)
		RegisterToStaticMap(comp);
	m_PendingStaticRegistrations.clear();

	UpdateCollisionMap();
	++m_FrameId;

	for (auto& [loc, vec] : m_DynamicCollisionMap) {
		auto staticIter = m_StaticCollisionMap.find(loc);
		const auto* staticVec = staticIter != m_StaticCollisionMap.end() ? &staticIter->second : nullptr;

		for (size_t i = 0; i < vec.size(); ++i) {
			auto* A = vec[i];
			if (staticVec) {
				for (auto* B : *staticVec) {
					if (A == B) {
						continue;
					}
					if (A > B) {
						if (!B->ShouldProcessPair(A, m_FrameId)) {
							continue;
						}
					}
					else {
						if (!A->ShouldProcessPair(B, m_FrameId)) {
							continue;
						}
					}
					CheckCollisionPair(A, B);
				}
			}

			for (size_t j = i + 1; j < vec.size(); ++j) {
				auto* B = vec[j];
				if (A > B) {
					if (!B->ShouldProcessPair(A, m_FrameId)) {
						continue;
					}
				}
				else {
					if (!A->ShouldProcessPair(B, m_FrameId)) {
						continue;
					}
				}
				CheckCollisionPair(A, B);
			}
		}
	}
}

void CollisionSystem::CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b)
{
	FVector2D locA = a->GetWorldLocation(), locB = b->GetWorldLocation();
	float radA = a->GetRadius() * a->GetScale(), radB = b->GetRadius() * b->GetScale();
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
			if (distance <= 0.0001f) {
				return;
			}

			FVector2D normal = { dx / distance, dy / distance };

			float overlapDepth = minDistance - distance;

			CollisionResolution(aActor, bActor, normal, overlapDepth);
		}
	}
	else {
		a->UpdateOverlapState(bActor, false);
		b->UpdateOverlapState(aActor, false);
	}

}

static float ClampFloat(float value, float minValue, float maxValue)
{
	return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

static float DistanceSquaredPointToSegment(const FVector2D& point, const FVector2D& a, const FVector2D& b)
{
	FVector2D ab{ b.X - a.X, b.Y - a.Y };
	FVector2D ap{ point.X - a.X, point.Y - a.Y };
	float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
	if (abLenSq <= 0.0001f) {
		return ap.X * ap.X + ap.Y * ap.Y;
	}
	float t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
	t = ClampFloat(t, 0.0f, 1.0f);
	FVector2D closest{ a.X + ab.X * t, a.Y + ab.Y * t };
	float dx = point.X - closest.X;
	float dy = point.Y - closest.Y;
	return dx * dx + dy * dy;
}

static bool LineSegmentsIntersect(const FVector2D& a1, const FVector2D& a2, const FVector2D& b1, const FVector2D& b2)
{
	FVector2D r{ a2.X - a1.X, a2.Y - a1.Y };
	FVector2D s{ b2.X - b1.X, b2.Y - b1.Y };
	float rxs = r.X * s.Y - r.Y * s.X;
	float qpxr = (b1.X - a1.X) * r.Y - (b1.Y - a1.Y) * r.X;
	if (std::abs(rxs) <= 0.0001f && std::abs(qpxr) <= 0.0001f) {
		float rdotr = r.X * r.X + r.Y * r.Y;
		float t0 = ((b1.X - a1.X) * r.X + (b1.Y - a1.Y) * r.Y) / rdotr;
		float t1 = t0 + (s.X * r.X + s.Y * r.Y) / rdotr;
		if (t0 > t1) {
			std::swap(t0, t1);
		}
		return t0 <= 1.0f && t1 >= 0.0f;
	}
	if (std::abs(rxs) <= 0.0001f) {
		return false;
	}
	float t = ((b1.X - a1.X) * s.Y - (b1.Y - a1.Y) * s.X) / rxs;
	float u = ((b1.X - a1.X) * r.Y - (b1.Y - a1.Y) * r.X) / rxs;
	return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
}

void CollisionSystem::CircleAndRectangle(MCircleCollisionComponent* circle, MRectangleCollisionComponent* rect)
{
	FVector2D circleCenter = circle->GetWorldLocation();
	FVector2D rectCenter = rect->GetWorldLocation();
	float halfWidth = (rect->GetWidth() * rect->GetScale()) * 0.5f;
	float halfHeight = (rect->GetHeight() * rect->GetScale()) * 0.5f;
	float minX = rectCenter.X - halfWidth;
	float maxX = rectCenter.X + halfWidth;
	float minY = rectCenter.Y - halfHeight;
	float maxY = rectCenter.Y + halfHeight;

	float closestX = ClampFloat(circleCenter.X, minX, maxX);
	float closestY = ClampFloat(circleCenter.Y, minY, maxY);
	float dx = circleCenter.X - closestX;
	float dy = circleCenter.Y - closestY;
	float radius = circle->GetRadius() * circle->GetScale();
	bool isOverlapping = (dx * dx + dy * dy) <= (radius * radius);

	auto circleActor = circle->GetOwner();
	auto rectActor = rect->GetOwner();
	circle->UpdateOverlapState(rectActor, isOverlapping);
	rect->UpdateOverlapState(circleActor, isOverlapping);
	if (isOverlapping
		&& circle->GetCollisionType() == ECollisionType::Block
		&& rect->GetCollisionType() == ECollisionType::Block) {
		FVector2D normal{ 0.0f, 0.0f };
		float overlapDepth = 0.0f;
		float distanceSquared = dx * dx + dy * dy;
		if (distanceSquared <= 0.0001f) {
			float left = circleCenter.X - minX;
			float right = maxX - circleCenter.X;
			float top = circleCenter.Y - minY;
			float bottom = maxY - circleCenter.Y;
			float minEdge = left;
			normal = { -1.0f, 0.0f };
			if (right < minEdge) {
				minEdge = right;
				normal = { 1.0f, 0.0f };
			}
			if (top < minEdge) {
				minEdge = top;
				normal = { 0.0f, -1.0f };
			}
			if (bottom < minEdge) {
				minEdge = bottom;
				normal = { 0.0f, 1.0f };
			}
			overlapDepth = radius + minEdge;
		}
		else {
			float distance = std::sqrt(distanceSquared);
			normal = { dx / distance, dy / distance };
			overlapDepth = radius - distance;
		}


		CollisionResolution(circleActor, rectActor, normal, overlapDepth);
	}
}

void CollisionSystem::RectangleAndRectangle(MRectangleCollisionComponent* a, MRectangleCollisionComponent* b)
{
	FVector2D aCenter = a->GetWorldLocation();
	FVector2D bCenter = b->GetWorldLocation();
	float aHalfW = (a->GetWidth() * a->GetScale()) * 0.5f;
	float aHalfH = (a->GetHeight() * a->GetScale()) * 0.5f;
	float bHalfW = (b->GetWidth() * b->GetScale()) * 0.5f;
	float bHalfH = (b->GetHeight() * b->GetScale()) * 0.5f;

	bool isOverlapping = std::abs(aCenter.X - bCenter.X) <= (aHalfW + bHalfW)
		&& std::abs(aCenter.Y - bCenter.Y) <= (aHalfH + bHalfH);

	auto aActor = a->GetOwner();
	auto bActor = b->GetOwner();
	a->UpdateOverlapState(bActor, isOverlapping);
	b->UpdateOverlapState(aActor, isOverlapping);
	if (isOverlapping
		&& a->GetCollisionType() == ECollisionType::Block
		&& b->GetCollisionType() == ECollisionType::Block) {
		float dx = bCenter.X - aCenter.X;
		float dy = bCenter.Y - aCenter.Y;
		float overlapX = (aHalfW + bHalfW) - std::abs(dx);
		float overlapY = (aHalfH + bHalfH) - std::abs(dy);
		if (overlapX <= 0.0f || overlapY <= 0.0f) {
			return;
		}
		FVector2D normal{ 0.0f, 0.0f };
		float overlapDepth = 0.0f;
		if (overlapX < overlapY) {
			normal = { dx >= 0.0f ? 1.0f : -1.0f, 0.0f };
			overlapDepth = overlapX;
		}
		else {
			normal = { 0.0f, dy >= 0.0f ? 1.0f : -1.0f };
			overlapDepth = overlapY;
		}

		CollisionResolution(aActor, bActor, normal, overlapDepth);
	}
}

void CollisionSystem::LineAndCircle(MLineCollisionComponent* line, MCircleCollisionComponent* circle)
{
	FVector2D start = line->GetWorldStart();
	FVector2D end = line->GetWorldEnd();
	FVector2D center = circle->GetWorldLocation();
	float radius = circle->GetRadius() * circle->GetScale();
	bool isOverlapping = DistanceSquaredPointToSegment(center, start, end) <= (radius * radius);

	auto lineActor = line->GetOwner();
	auto circleActor = circle->GetOwner();
	line->UpdateOverlapState(circleActor, isOverlapping);
	circle->UpdateOverlapState(lineActor, isOverlapping);
	if (isOverlapping
		&& line->GetCollisionType() == ECollisionType::Block
		&& circle->GetCollisionType() == ECollisionType::Block) {
		FVector2D ab{ end.X - start.X, end.Y - start.Y };
		FVector2D ap{ center.X - start.X, center.Y - start.Y };
		float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
		float t = 0.0f;
		if (abLenSq > 0.0001f) {
			t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
			t = ClampFloat(t, 0.0f, 1.0f);
		}
		FVector2D closest{ start.X + ab.X * t, start.Y + ab.Y * t };
		float dx = center.X - closest.X;
		float dy = center.Y - closest.Y;
		float distanceSquared = dx * dx + dy * dy;
		if (distanceSquared <= 0.0001f) {
			return;
		}
		float distance = std::sqrt(distanceSquared);
		FVector2D normal{ dx / distance, dy / distance };
		float overlapDepth = radius - distance;

		CollisionResolution(lineActor, circleActor, normal, overlapDepth);
	}
}

void CollisionSystem::LineAndRectangle(MLineCollisionComponent* line, MRectangleCollisionComponent* rect)
{
	FVector2D start = line->GetWorldStart();
	FVector2D end = line->GetWorldEnd();
	FVector2D rectCenter = rect->GetWorldLocation();
	float halfWidth = (rect->GetWidth() * rect->GetScale()) * 0.5f;
	float halfHeight = (rect->GetHeight() * rect->GetScale()) * 0.5f;

	float minX = rectCenter.X - halfWidth;
	float maxX = rectCenter.X + halfWidth;
	float minY = rectCenter.Y - halfHeight;
	float maxY = rectCenter.Y + halfHeight;

	bool startInside = start.X >= minX && start.X <= maxX && start.Y >= minY && start.Y <= maxY;
	bool endInside = end.X >= minX && end.X <= maxX && end.Y >= minY && end.Y <= maxY;
	bool isOverlapping = startInside || endInside;
	if (!isOverlapping) {
		FVector2D topLeft{ minX, minY };
		FVector2D topRight{ maxX, minY };
		FVector2D bottomLeft{ minX, maxY };
		FVector2D bottomRight{ maxX, maxY };
		isOverlapping = LineSegmentsIntersect(start, end, topLeft, topRight)
			|| LineSegmentsIntersect(start, end, topRight, bottomRight)
			|| LineSegmentsIntersect(start, end, bottomRight, bottomLeft)
			|| LineSegmentsIntersect(start, end, bottomLeft, topLeft);
	}

	auto lineActor = line->GetOwner();
	auto rectActor = rect->GetOwner();
	line->UpdateOverlapState(rectActor, isOverlapping);
	rect->UpdateOverlapState(lineActor, isOverlapping);
	if (isOverlapping
		&& line->GetCollisionType() == ECollisionType::Block
		&& rect->GetCollisionType() == ECollisionType::Block) {
		FVector2D ab{ end.X - start.X, end.Y - start.Y };
		FVector2D ap{ rectCenter.X - start.X, rectCenter.Y - start.Y };
		float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
		float t = 0.0f;
		if (abLenSq > 0.0001f) {
			t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
			t = ClampFloat(t, 0.0f, 1.0f);
		}
		FVector2D closest{ start.X + ab.X * t, start.Y + ab.Y * t };
		float dx = rectCenter.X - closest.X;
		float dy = rectCenter.Y - closest.Y;
		float distanceSquared = dx * dx + dy * dy;
		if (distanceSquared <= 0.0001f) {
			return;
		}
		float distance = std::sqrt(distanceSquared);
		FVector2D normal{ dx / distance, dy / distance };
		float supportRadius = std::abs(normal.X) * halfWidth + std::abs(normal.Y) * halfHeight;
		float overlapDepth = supportRadius - distance;
		if (overlapDepth <= 0.0f) {
			return;
		}

		CollisionResolution(lineActor, rectActor, normal, overlapDepth);
	}
}

void CollisionSystem::LineAndLine(MLineCollisionComponent* a, MLineCollisionComponent* b)
{
	FVector2D aStart = a->GetWorldStart();
	FVector2D aEnd = a->GetWorldEnd();
	FVector2D bStart = b->GetWorldStart();
	FVector2D bEnd = b->GetWorldEnd();
	bool isOverlapping = LineSegmentsIntersect(aStart, aEnd, bStart, bEnd);

	auto aActor = a->GetOwner();
	auto bActor = b->GetOwner();
	a->UpdateOverlapState(bActor, isOverlapping);
	b->UpdateOverlapState(aActor, isOverlapping);
}

void CollisionSystem::CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal)
{
	FVector2D v = move->GetVelocity();
	float dot = v.X * normal.X + v.Y * normal.Y;
	if (dot < 0.0f) { // 衝突方向に向かっている成分のみキャンセル
		move->SetWorldForce({ v.X - normal.X * dot, v.Y - normal.Y * dot });
	}
}

void CollisionSystem::CollisionResolution(AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth)
{
	auto moveComponentA = ActorA->GetComponents<MMovementComponent>();
	auto moveComponentB = ActorB->GetComponents<MMovementComponent>();
	auto* moveA = moveComponentA.empty() ? nullptr : moveComponentA.front();
	auto* moveB = moveComponentB.empty() ? nullptr : moveComponentB.front();

	if (moveA && moveB) {
		ActorA->AddActorWorldOffset(normal * (-overlapDepth * 0.5f));
		ActorB->AddActorWorldOffset(normal * (overlapDepth * 0.5f));
		CancelNormalVelocity(moveA, normal);
		CancelNormalVelocity(moveB, { -normal.X, -normal.Y });
	}
	else if (moveA) {
		ActorA->AddActorWorldOffset(normal * (overlapDepth));
		CancelNormalVelocity(moveA, normal);
	}
	else if (moveB) {
		ActorB->AddActorWorldOffset(normal * (-overlapDepth));
		CancelNormalVelocity(moveB, { -normal.X, -normal.Y });
	}

}


void CollisionSystem::CheckCollisionPair(MCollisionComponent* A, MCollisionComponent* B)
{
	if (A->GetOwner() == B->GetOwner()) return;
	if (A->IsStatic() && B->IsStatic())return;
	switch (A->GetShapeType()) {
	case ECollisionShape::Circle:
		if (B->GetShapeType() == ECollisionShape::Circle) {
			CircleAndCircle(static_cast<MCircleCollisionComponent*>(A), static_cast<MCircleCollisionComponent*>(B));
		}
		else if (B->GetShapeType() == ECollisionShape::Rectangle) {
			CircleAndRectangle(static_cast<MCircleCollisionComponent*>(A), static_cast<MRectangleCollisionComponent*>(B));
		}
		else if (B->GetShapeType() == ECollisionShape::Line) {
			LineAndCircle(static_cast<MLineCollisionComponent*>(B), static_cast<MCircleCollisionComponent*>(A));
		}
		break;
	case ECollisionShape::Rectangle:
		if (B->GetShapeType() == ECollisionShape::Circle) {
			CircleAndRectangle(static_cast<MCircleCollisionComponent*>(B), static_cast<MRectangleCollisionComponent*>(A));
		}
		else if (B->GetShapeType() == ECollisionShape::Rectangle) {
			RectangleAndRectangle(static_cast<MRectangleCollisionComponent*>(A), static_cast<MRectangleCollisionComponent*>(B));
		}
		else if (B->GetShapeType() == ECollisionShape::Line) {
			LineAndRectangle(static_cast<MLineCollisionComponent*>(B), static_cast<MRectangleCollisionComponent*>(A));
		}
		break;
	case ECollisionShape::Line:
		if (B->GetShapeType() == ECollisionShape::Circle) {
			LineAndCircle(static_cast<MLineCollisionComponent*>(A), static_cast<MCircleCollisionComponent*>(B));
		}
		else if (B->GetShapeType() == ECollisionShape::Rectangle) {
			LineAndRectangle(static_cast<MLineCollisionComponent*>(A), static_cast<MRectangleCollisionComponent*>(B));
		}
		else if (B->GetShapeType() == ECollisionShape::Line) {
			LineAndLine(static_cast<MLineCollisionComponent*>(A), static_cast<MLineCollisionComponent*>(B));
		}
		break;
	}
}

void CollisionSystem::RegisterToStaticMap(MCollisionComponent* component)
{
	FAABB box = component->GetAABB();
	int minX = static_cast<int>(std::floor(box.MinX / m_CollisionCellSize));
	int maxX = static_cast<int>(std::floor(box.MaxX / m_CollisionCellSize));
	int minY = static_cast<int>(std::floor(box.MinY / m_CollisionCellSize));
	int maxY = static_cast<int>(std::floor(box.MaxY / m_CollisionCellSize));

	for (int x = minX; x <= maxX; ++x) {
		for (int y = minY; y <= maxY; ++y) {
			m_StaticCollisionMap[{x, y}].push_back(component);
		}
	}
	
}

