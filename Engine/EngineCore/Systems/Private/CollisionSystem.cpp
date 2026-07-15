#include "CollisionSystem.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Actor.h"
#include "CircleCollisionComponent.h"
#include "LineCollisionComponent.h"
#include "MovementComponent.h"
#include "RectangleCollisionComponent.h"

struct pair_hash {
  inline std::size_t operator()(const std::pair<int, int>& v) const {
    return v.first * 31 + v.second;
  }
};

struct FCollisionSystem::Impl {
  std::vector<MCollisionComponent*> CollisionComponents;

  std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash>
      StaticCollisionMap;
  std::unordered_map<std::pair<int, int>, std::vector<MCollisionComponent*>, pair_hash>
      DynamicCollisionMap;

  std::vector<std::pair<int, int>> ActiveStaticCells;
  std::vector<std::pair<int, int>> ActiveDynamicCells;

  std::unordered_map<MCollisionComponent*, FAABB> CachedAABBs;

  float CollisionCellSize = 100;
  std::uint64_t FrameId = 0;

  bool bDeferStaticRebuild = false;
  bool bPendingStaticRebuild = false;
  std::vector<MCollisionComponent*> PendingStaticRegistrations;
};
FCollisionSystem::FCollisionSystem() : ImplPtr(new Impl()) {}
FCollisionSystem::~FCollisionSystem() { delete ImplPtr; }

float FCollisionSystem::GetCollisionCellSize() const { return ImplPtr->CollisionCellSize; }

void FCollisionSystem::RegisterCollision(MCollisionComponent* component) {
  ImplPtr->CollisionComponents.push_back(component);
  if (component->IsStatic()) {
    if (std::find(
            ImplPtr->PendingStaticRegistrations.begin(),
            ImplPtr->PendingStaticRegistrations.end(),
            component
        ) == ImplPtr->PendingStaticRegistrations.end()) {
      ImplPtr->PendingStaticRegistrations.push_back(component);
    }
  }
}

void FCollisionSystem::UnRegisterCollision(MCollisionComponent* component) {
  auto it = std::find(
      ImplPtr->CollisionComponents.begin(), ImplPtr->CollisionComponents.end(), component
  );
  if (it != ImplPtr->CollisionComponents.end()) {
    *it = ImplPtr->CollisionComponents.back();
    ImplPtr->CollisionComponents.pop_back();
  }

  ImplPtr->PendingStaticRegistrations.erase(
      std::remove(
          ImplPtr->PendingStaticRegistrations.begin(),
          ImplPtr->PendingStaticRegistrations.end(),
          component
      ),
      ImplPtr->PendingStaticRegistrations.end()
  );

  ImplPtr->CachedAABBs.erase(component);

  if (component->IsStatic()) {
    if (ImplPtr->bDeferStaticRebuild) {
      ImplPtr->bPendingStaticRebuild = true;
    } else {
      RebuildStaticCollisionMap();
    }
  }
}

void FCollisionSystem::BeginSceneTransition() { ImplPtr->bDeferStaticRebuild = true; }

void FCollisionSystem::EndSceneTransition() {
  ImplPtr->bDeferStaticRebuild = false;
  if (ImplPtr->bPendingStaticRebuild) {
    RebuildStaticCollisionMap();
    ImplPtr->bPendingStaticRebuild = false;
  }
}

void FCollisionSystem::RebuildStaticCollisionMap() {
  for (const auto& loc : ImplPtr->ActiveStaticCells) {
    ImplPtr->StaticCollisionMap[loc].clear();
  }
  ImplPtr->ActiveStaticCells.clear();

  for (auto* comp : ImplPtr->CollisionComponents) {
    if (!comp->IsStatic()) continue;
    ImplPtr->CachedAABBs[comp] = comp->GetAABB();
    RegisterToStaticMap(comp);
    comp->SetGridClean();
  }
}

void FCollisionSystem::UpdateCollisionMap() {
  for (const auto& loc : ImplPtr->ActiveDynamicCells) {
    ImplPtr->DynamicCollisionMap[loc].clear();
  }
  ImplPtr->ActiveDynamicCells.clear();

  for (auto collision : ImplPtr->CollisionComponents) {
    if (collision->IsStatic()) {
      continue;
    }

    const FAABB& box = ImplPtr->CachedAABBs[collision];
    int minX = static_cast<int>(std::floor(box.MinX / ImplPtr->CollisionCellSize));
    int maxX = static_cast<int>(std::floor(box.MaxX / ImplPtr->CollisionCellSize));
    int minY = static_cast<int>(std::floor(box.MinY / ImplPtr->CollisionCellSize));
    int maxY = static_cast<int>(std::floor(box.MaxY / ImplPtr->CollisionCellSize));

    for (int x = minX; x <= maxX; ++x) {
      for (int y = minY; y <= maxY; ++y) {
        auto loc = std::make_pair(x, y);
        if (ImplPtr->DynamicCollisionMap[loc].empty()) {
          ImplPtr->ActiveDynamicCells.push_back(loc);
        }
        ImplPtr->DynamicCollisionMap[loc].push_back(collision);
      }
    }
    collision->SetGridClean();
  }
}

void FCollisionSystem::CheckCollisions() {
  for (auto* comp : ImplPtr->CollisionComponents) {
    ImplPtr->CachedAABBs[comp] = comp->GetAABB();
  }

  for (auto* comp : ImplPtr->PendingStaticRegistrations) {
    RegisterToStaticMap(comp);
  }
  ImplPtr->PendingStaticRegistrations.clear();

  UpdateCollisionMap();
  ++ImplPtr->FrameId;

  for (const auto& loc : ImplPtr->ActiveDynamicCells) {
    const auto& vec = ImplPtr->DynamicCollisionMap[loc];
    auto staticIter = ImplPtr->StaticCollisionMap.find(loc);
    const auto* staticVec =
        staticIter != ImplPtr->StaticCollisionMap.end() ? &staticIter->second : nullptr;

    for (size_t i = 0; i < vec.size(); ++i) {
      auto* A = vec[i];
      const FAABB& aabbA = ImplPtr->CachedAABBs[A];

      if (staticVec) {
        for (auto* B : *staticVec) {
          if (A == B) {
            continue;
          }

          const FAABB& aabbB = ImplPtr->CachedAABBs[B];
          if (aabbA.MaxX < aabbB.MinX || aabbA.MinX > aabbB.MaxX || aabbA.MaxY < aabbB.MinY ||
              aabbA.MinY > aabbB.MaxY) {
            continue;
          }

          if (A > B) {
            if (!B->ShouldProcessPair(A, ImplPtr->FrameId)) continue;
          } else {
            if (!A->ShouldProcessPair(B, ImplPtr->FrameId)) continue;
          }
          CheckCollisionPair(A, B);
        }
      }

      for (size_t j = i + 1; j < vec.size(); ++j) {
        auto* B = vec[j];
        const FAABB& aabbB = ImplPtr->CachedAABBs[B];
        if (aabbA.MaxX < aabbB.MinX || aabbA.MinX > aabbB.MaxX || aabbA.MaxY < aabbB.MinY ||
            aabbA.MinY > aabbB.MaxY) {
          continue;
        }

        if (A > B) {
          if (!B->ShouldProcessPair(A, ImplPtr->FrameId)) continue;
        } else {
          if (!A->ShouldProcessPair(B, ImplPtr->FrameId)) continue;
        }
        CheckCollisionPair(A, B);
      }
    }
  }

  for (auto* comp : ImplPtr->CollisionComponents) {
    comp->FlushOverlapState();
  }
}

void FCollisionSystem::RemoveActorReferences(AActor* Actor) {
  if (!Actor) return;
  for (auto* comp : ImplPtr->CollisionComponents) {
    if (comp) {
      comp->RemoveActorReference(Actor);
    }
  }
}

void FCollisionSystem::CircleAndCircle(MCircleCollisionComponent* a, MCircleCollisionComponent* b) {
  FVector2D locA = a->GetWorldLocation(), locB = b->GetWorldLocation();
  float radA = a->GetRadius() * a->GetWorldScale().Scale,
        radB = b->GetRadius() * b->GetWorldScale().Scale;
  float dx = locB.X - locA.X;
  float dy = locB.Y - locA.Y;
  float distanceSquared = dx * dx + dy * dy;
  float minDistance = radA + radB;
  auto aActor = a->GetOwner();
  auto bActor = b->GetOwner();
  if (distanceSquared <= minDistance * minDistance) {
    a->UpdateOverlapState(bActor, true);
    b->UpdateOverlapState(aActor, true);
    if (a->GetCollisionType() == ECollisionType::Block &&
        b->GetCollisionType() == ECollisionType::Block) {
      float distance = std::sqrt(distanceSquared);
      FVector2D normal;
      if (distance <= 0.0001f) {
        normal = {1.0f, 0.0f};
      } else {
        normal = {dx / distance, dy / distance};
      }
      float overlapDepth = minDistance - distance;
      CollisionResolution(aActor, bActor, normal, overlapDepth);
    }
  } else {
    a->UpdateOverlapState(bActor, false);
    b->UpdateOverlapState(aActor, false);
  }
}

static float ClampFloat(float value, float minValue, float maxValue) {
  return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

static float DistanceSquaredPointToSegment(
    const FVector2D& point, const FVector2D& a, const FVector2D& b
) {
  FVector2D ab{b.X - a.X, b.Y - a.Y};
  FVector2D ap{point.X - a.X, point.Y - a.Y};
  float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
  if (abLenSq <= 0.0001f) {
    return ap.X * ap.X + ap.Y * ap.Y;
  }
  float t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
  t = ClampFloat(t, 0.0f, 1.0f);
  FVector2D closest{a.X + ab.X * t, a.Y + ab.Y * t};
  float dx = point.X - closest.X;
  float dy = point.Y - closest.Y;
  return dx * dx + dy * dy;
}

static bool LineSegmentsIntersect(
    const FVector2D& a1, const FVector2D& a2, const FVector2D& b1, const FVector2D& b2
) {
  FVector2D r{a2.X - a1.X, a2.Y - a1.Y};
  FVector2D s{b2.X - b1.X, b2.Y - b1.Y};
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

void FCollisionSystem::CircleAndRectangle(
    MCircleCollisionComponent* circle, MRectangleCollisionComponent* rect
) {
  FVector2D circleCenter = circle->GetWorldLocation();
  FVector2D rectCenter = rect->GetWorldLocation();
  float halfWidth = (rect->GetWidth() * rect->GetWorldScale().Scale) * 0.5f;
  float halfHeight = (rect->GetHeight() * rect->GetWorldScale().Scale) * 0.5f;
  float rad = UMath::DegToRad(rect->GetWorldRotation().Rotation);
  float cosA = std::cos(rad);
  float sinA = std::sin(rad);

  float dx = circleCenter.X - rectCenter.X;
  float dy = circleCenter.Y - rectCenter.Y;
  float localCircleX = dx * cosA + dy * sinA;
  float localCircleY = -dx * sinA + dy * cosA;
  float closestX = ClampFloat(localCircleX, -halfWidth, halfWidth);
  float closestY = ClampFloat(localCircleY, -halfHeight, halfHeight);
  float localDx = localCircleX - closestX;
  float localDy = localCircleY - closestY;
  float distanceSquared = localDx * localDx + localDy * localDy;
  float radius = circle->GetRadius() * circle->GetWorldScale().Scale;
  auto circleActor = circle->GetOwner();
  auto rectActor = rect->GetOwner();
  constexpr float HysteresisMargin = 1.0f;
  float exitRadius = radius + (circle->IsOverlappingActor(rectActor) ? HysteresisMargin : 0.0f);
  bool isOverlapping = distanceSquared <= (exitRadius * exitRadius);

  circle->UpdateOverlapState(rectActor, isOverlapping);
  rect->UpdateOverlapState(circleActor, isOverlapping);

  if (isOverlapping && circle->GetCollisionType() == ECollisionType::Block &&
      rect->GetCollisionType() == ECollisionType::Block) {
    FVector2D localNormal{0.0f, 0.0f};
    float overlapDepth = 0.0f;
    if (distanceSquared <= 0.0001f) {
      float left = localCircleX - (-halfWidth);
      float right = halfWidth - localCircleX;
      float top = localCircleY - (-halfHeight);
      float bottom = halfHeight - localCircleY;
      float minEdge = left;
      localNormal = {-1.0f, 0.0f};
      if (right < minEdge) {
        minEdge = right;
        localNormal = {1.0f, 0.0f};
      }
      if (top < minEdge) {
        minEdge = top;
        localNormal = {0.0f, -1.0f};
      }
      if (bottom < minEdge) {
        minEdge = bottom;
        localNormal = {0.0f, 1.0f};
      }
      overlapDepth = radius + minEdge;
    } else {
      float distance = std::sqrt(distanceSquared);
      localNormal = {localDx / distance, localDy / distance};
      overlapDepth = radius - distance;
    }
    FVector2D worldNormal = {
        localNormal.X * cosA - localNormal.Y * sinA, localNormal.X * sinA + localNormal.Y * cosA
    };
    CollisionResolution(circleActor, rectActor, {-worldNormal.X, -worldNormal.Y}, overlapDepth);
  }
}

struct OBB {
  FVector2D Center;
  FVector2D AxisX;
  FVector2D AxisY;
  float HalfW;
  float HalfH;
};

void FCollisionSystem::RectangleAndRectangle(
    MRectangleCollisionComponent* a, MRectangleCollisionComponent* b
) {
  auto GetOBB = [](MRectangleCollisionComponent* rect) -> OBB {
    OBB obb;
    obb.Center = rect->GetWorldLocation();
    float rad = UMath::DegToRad(rect->GetWorldRotation().Rotation);
    obb.AxisX = {std::cos(rad), std::sin(rad)};
    obb.AxisY = {-std::sin(rad), std::cos(rad)};
    obb.HalfW = (rect->GetWidth() * rect->GetWorldScale().Scale) * 0.5f;
    obb.HalfH = (rect->GetHeight() * rect->GetWorldScale().Scale) * 0.5f;
    return obb;
  };

  OBB obbA = GetOBB(a);
  OBB obbB = GetOBB(b);
  FVector2D axes[4] = {obbA.AxisX, obbA.AxisY, obbB.AxisX, obbB.AxisY};
  float minOverlap = 1e9f;
  FVector2D mtvAxis{0.0f, 0.0f};
  bool isOverlapping = true;

  for (int i = 0; i < 4; ++i) {
    FVector2D axis = axes[i];
    if (axis.X * axis.X + axis.Y * axis.Y < 0.0001f) continue;
    auto Project = [](const OBB& obb, const FVector2D& ax) -> float {
      return std::abs(obb.AxisX.X * ax.X + obb.AxisX.Y * ax.Y) * obb.HalfW +
             std::abs(obb.AxisY.X * ax.X + obb.AxisY.Y * ax.Y) * obb.HalfH;
    };
    float rA = Project(obbA, axis);
    float rB = Project(obbB, axis);
    FVector2D t = {obbB.Center.X - obbA.Center.X, obbB.Center.Y - obbA.Center.Y};
    float distance = std::abs(t.X * axis.X + t.Y * axis.Y);
    float overlap = rA + rB - distance;

    if (overlap <= 0.0f) {
      isOverlapping = false;
      break;
    }
    if (overlap < minOverlap) {
      minOverlap = overlap;
      mtvAxis = axis;
      if (t.X * axis.X + t.Y * axis.Y < 0.0f) {
        mtvAxis = {-axis.X, -axis.Y};
      }
    }
  }

  auto aActor = a->GetOwner();
  auto bActor = b->GetOwner();
  a->UpdateOverlapState(bActor, isOverlapping);
  b->UpdateOverlapState(aActor, isOverlapping);

  if (isOverlapping && a->GetCollisionType() == ECollisionType::Block &&
      b->GetCollisionType() == ECollisionType::Block) {
    CollisionResolution(aActor, bActor, mtvAxis, minOverlap);
  }
}

void FCollisionSystem::LineAndCircle(
    MLineCollisionComponent* line, MCircleCollisionComponent* circle
) {
  FVector2D start = line->GetWorldStart();
  FVector2D end = line->GetWorldEnd();
  FVector2D center = circle->GetWorldLocation();
  float radius = circle->GetRadius() * circle->GetWorldScale().Scale;
  bool isOverlapping = DistanceSquaredPointToSegment(center, start, end) <= (radius * radius);

  auto lineActor = line->GetOwner();
  auto circleActor = circle->GetOwner();
  line->UpdateOverlapState(circleActor, isOverlapping);
  circle->UpdateOverlapState(lineActor, isOverlapping);

  if (isOverlapping && line->GetCollisionType() == ECollisionType::Block &&
      circle->GetCollisionType() == ECollisionType::Block) {
    FVector2D ab{end.X - start.X, end.Y - start.Y};
    FVector2D ap{center.X - start.X, center.Y - start.Y};
    float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
    float t = 0.0f;
    if (abLenSq > 0.0001f) {
      t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
      t = ClampFloat(t, 0.0f, 1.0f);
    }
    FVector2D closest{start.X + ab.X * t, start.Y + ab.Y * t};
    float dx = center.X - closest.X;
    float dy = center.Y - closest.Y;
    float distanceSquared = dx * dx + dy * dy;

    if (distanceSquared <= 0.0001f) return;
    float distance = std::sqrt(distanceSquared);
    FVector2D normal{dx / distance, dy / distance};
    float overlapDepth = radius - distance;
    CollisionResolution(lineActor, circleActor, normal, overlapDepth);
  }
}

void FCollisionSystem::LineAndRectangle(
    MLineCollisionComponent* line, MRectangleCollisionComponent* rect
) {
  FVector2D start = line->GetWorldStart();
  FVector2D end = line->GetWorldEnd();
  FVector2D rectCenter = rect->GetWorldLocation();
  float halfWidth = (rect->GetWidth() * rect->GetWorldScale().Scale) * 0.5f;
  float halfHeight = (rect->GetHeight() * rect->GetWorldScale().Scale) * 0.5f;
  float rad = UMath::DegToRad(rect->GetWorldRotation().Rotation);
  float cosA = std::cos(rad);
  float sinA = std::sin(rad);

  auto ToLocal = [&](const FVector2D& pt) -> FVector2D {
    float dx = pt.X - rectCenter.X;
    float dy = pt.Y - rectCenter.Y;
    return {dx * cosA + dy * sinA, -dx * sinA + dy * cosA};
  };

  FVector2D localStart = ToLocal(start);
  FVector2D localEnd = ToLocal(end);
  float minX = -halfWidth;
  float maxX = halfWidth;
  float minY = -halfHeight;
  float maxY = halfHeight;

  bool startInside =
      localStart.X >= minX && localStart.X <= maxX && localStart.Y >= minY && localStart.Y <= maxY;
  bool endInside =
      localEnd.X >= minX && localEnd.X <= maxX && localEnd.Y >= minY && localEnd.Y <= maxY;
  bool isOverlapping = startInside || endInside;

  if (!isOverlapping) {
    FVector2D topLeft{minX, minY};
    FVector2D topRight{maxX, minY};
    FVector2D bottomLeft{minX, maxY};
    FVector2D bottomRight{maxX, maxY};
    isOverlapping = LineSegmentsIntersect(localStart, localEnd, topLeft, topRight) ||
                    LineSegmentsIntersect(localStart, localEnd, topRight, bottomRight) ||
                    LineSegmentsIntersect(localStart, localEnd, bottomRight, bottomLeft) ||
                    LineSegmentsIntersect(localStart, localEnd, bottomLeft, topLeft);
  }

  auto lineActor = line->GetOwner();
  auto rectActor = rect->GetOwner();
  line->UpdateOverlapState(rectActor, isOverlapping);
  rect->UpdateOverlapState(lineActor, isOverlapping);

  if (isOverlapping && line->GetCollisionType() == ECollisionType::Block &&
      rect->GetCollisionType() == ECollisionType::Block) {
    FVector2D ab{localEnd.X - localStart.X, localEnd.Y - localStart.Y};
    FVector2D ap{-localStart.X, -localStart.Y};
    float abLenSq = ab.X * ab.X + ab.Y * ab.Y;
    float t = 0.0f;
    if (abLenSq > 0.0001f) {
      t = (ap.X * ab.X + ap.Y * ab.Y) / abLenSq;
      t = ClampFloat(t, 0.0f, 1.0f);
    }
    FVector2D closest{localStart.X + ab.X * t, localStart.Y + ab.Y * t};
    float dx = -closest.X;
    float dy = -closest.Y;
    float distanceSquared = dx * dx + dy * dy;

    if (distanceSquared <= 0.0001f) return;
    float distance = std::sqrt(distanceSquared);
    FVector2D localNormal{dx / distance, dy / distance};
    float supportRadius =
        std::abs(localNormal.X) * halfWidth + std::abs(localNormal.Y) * halfHeight;
    float overlapDepth = supportRadius - distance;
    if (overlapDepth <= 0.0f) return;

    FVector2D worldNormal = {
        localNormal.X * cosA - localNormal.Y * sinA, localNormal.X * sinA + localNormal.Y * cosA
    };
    CollisionResolution(lineActor, rectActor, worldNormal, overlapDepth);
  }
}

void FCollisionSystem::LineAndLine(MLineCollisionComponent* a, MLineCollisionComponent* b) {
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

void FCollisionSystem::CancelNormalVelocity(MMovementComponent* move, const FVector2D& normal) {
  FVector2D v = move->GetVelocity();
  float dot = v.X * normal.X + v.Y * normal.Y;
  if (dot > 0.0f) {
    move->SetWorldVelocity({v.X - normal.X * dot, v.Y - normal.Y * dot});
  }
}

void FCollisionSystem::CollisionResolution(
    AActor* ActorA, AActor* ActorB, const FVector2D& normal, float overlapDepth
) {
  auto moveComponentA = ActorA->GetComponents<MMovementComponent>();
  auto moveComponentB = ActorB->GetComponents<MMovementComponent>();
  auto* moveA = moveComponentA.empty() ? nullptr : moveComponentA.front();
  auto* moveB = moveComponentB.empty() ? nullptr : moveComponentB.front();

  bool bCanMoveA = moveA && (ActorA->bHasAuthority || ActorA->bIsLocallyControlled);
  bool bCanMoveB = moveB && (ActorB->bHasAuthority || ActorB->bIsLocallyControlled);

  if (bCanMoveA && bCanMoveB) {
    ActorA->AddActorWorldOffset(normal * (-overlapDepth * 0.5f));
    ActorB->AddActorWorldOffset(normal * (overlapDepth * 0.5f));
    CancelNormalVelocity(moveA, normal);
    CancelNormalVelocity(moveB, {-normal.X, -normal.Y});
  } else if (bCanMoveA) {
    ActorA->AddActorWorldOffset(normal * (-overlapDepth));
    CancelNormalVelocity(moveA, normal);
  } else if (bCanMoveB) {
    ActorB->AddActorWorldOffset(normal * (overlapDepth));
    CancelNormalVelocity(moveB, {-normal.X, -normal.Y});
  }
}

void FCollisionSystem::CheckCollisionPair(MCollisionComponent* A, MCollisionComponent* B) {
  if (A->GetOwner() == B->GetOwner()) return;
  if (A->IsStatic() && B->IsStatic()) return;

  switch (A->GetShapeType()) {
    case ECollisionShape::Circle:
      if (B->GetShapeType() == ECollisionShape::Circle) {
        CircleAndCircle(
            static_cast<MCircleCollisionComponent*>(A), static_cast<MCircleCollisionComponent*>(B)
        );
      } else if (B->GetShapeType() == ECollisionShape::Rectangle) {
        CircleAndRectangle(
            static_cast<MCircleCollisionComponent*>(A),
            static_cast<MRectangleCollisionComponent*>(B)
        );
      } else if (B->GetShapeType() == ECollisionShape::Line) {
        LineAndCircle(
            static_cast<MLineCollisionComponent*>(B), static_cast<MCircleCollisionComponent*>(A)
        );
      }
      break;
    case ECollisionShape::Rectangle:
      if (B->GetShapeType() == ECollisionShape::Circle) {
        CircleAndRectangle(
            static_cast<MCircleCollisionComponent*>(B),
            static_cast<MRectangleCollisionComponent*>(A)
        );
      } else if (B->GetShapeType() == ECollisionShape::Rectangle) {
        RectangleAndRectangle(
            static_cast<MRectangleCollisionComponent*>(A),
            static_cast<MRectangleCollisionComponent*>(B)
        );
      } else if (B->GetShapeType() == ECollisionShape::Line) {
        LineAndRectangle(
            static_cast<MLineCollisionComponent*>(B), static_cast<MRectangleCollisionComponent*>(A)
        );
      }
      break;
    case ECollisionShape::Line:
      if (B->GetShapeType() == ECollisionShape::Circle) {
        LineAndCircle(
            static_cast<MLineCollisionComponent*>(A), static_cast<MCircleCollisionComponent*>(B)
        );
      } else if (B->GetShapeType() == ECollisionShape::Rectangle) {
        LineAndRectangle(
            static_cast<MLineCollisionComponent*>(A), static_cast<MRectangleCollisionComponent*>(B)
        );
      } else if (B->GetShapeType() == ECollisionShape::Line) {
        LineAndLine(
            static_cast<MLineCollisionComponent*>(A), static_cast<MLineCollisionComponent*>(B)
        );
      }
      break;
  }
}

void FCollisionSystem::RegisterToStaticMap(MCollisionComponent* component) {
  const FAABB& box = ImplPtr->CachedAABBs[component];
  int minX = static_cast<int>(std::floor(box.MinX / ImplPtr->CollisionCellSize));
  int maxX = static_cast<int>(std::floor(box.MaxX / ImplPtr->CollisionCellSize));
  int minY = static_cast<int>(std::floor(box.MinY / ImplPtr->CollisionCellSize));
  int maxY = static_cast<int>(std::floor(box.MaxY / ImplPtr->CollisionCellSize));

  for (int x = minX; x <= maxX; ++x) {
    for (int y = minY; y <= maxY; ++y) {
      auto loc = std::make_pair(x, y);
      if (ImplPtr->StaticCollisionMap[loc].empty()) {
        ImplPtr->ActiveStaticCells.push_back(loc);
      }
      ImplPtr->StaticCollisionMap[loc].push_back(component);
    }
  }
}
