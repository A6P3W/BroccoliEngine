#pragma once
#include "SceneComponent.h"
#include "Actor.h"
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
enum class ECollisionShape
{
	Circle,
	Rectangle,
	Line,
};
enum class ECollisionType {
	Overlap,
	Block
};
struct FAABB {
	float MinX, MinY, MaxX, MaxY;
};
class MCollisionComponent : public MSceneComponent
{
public:
	MCollisionComponent();
	virtual ~MCollisionComponent();
	virtual ECollisionShape GetShapeType() const = 0;

	virtual FAABB GetAABB() const = 0;

	ECollisionType GetCollisionType() { return m_CollisionType; }
	void SetCollisionType(ECollisionType NewType) { m_CollisionType = NewType; }

	bool IsOverlappingActor(AActor* OtherActor) const {
		return m_OverlappingActors.contains(OtherActor);
	}
	bool ShouldProcessPair(MCollisionComponent* OtherComponent, std::uint64_t FrameId) {
		auto it = m_LastCheckedFrame.find(OtherComponent);
		if (it != m_LastCheckedFrame.end() && it->second == FrameId) {
			return false;
		}
		m_LastCheckedFrame[OtherComponent] = FrameId;
		return true;
	}
	void UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting) {
		if (bIsIntersecting) {
			if (m_OverlappingActors.insert(OtherActor).second) {
				GetOwner()->BeginOverlap(OtherActor);
			}
		}
		else {
			if (m_OverlappingActors.erase(OtherActor) > 0) {
				GetOwner()->EndOverlap(OtherActor);
			}
		}
	}
	void SetStatic(bool IsStatic);
	bool IsStatic() const { return bIsStatic; }
private:
	std::unordered_set<AActor*> m_OverlappingActors;
	std::unordered_map<MCollisionComponent*, std::uint64_t> m_LastCheckedFrame;
	ECollisionType m_CollisionType = ECollisionType::Overlap;
	bool bIsStatic = true;
};

