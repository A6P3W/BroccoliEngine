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
			m_CheckedThisFrame.insert(OtherComponent->GetOwner());
			return false;
		}

		m_LastCheckedFrame[OtherComponent] = FrameId;

		auto& otherFrame = OtherComponent->m_LastCheckedFrame;
		otherFrame[this] = FrameId;

		return true;
	}
	void UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting) {
		m_CheckedThisFrame.insert(OtherActor);
		if (bIsIntersecting) {
			m_IntersectingThisFrame.insert(OtherActor);
			if (m_OverlappingActors.insert(OtherActor).second) {
				GetOwner()->BeginOverlap(OtherActor);
			}
		}
	}
	void SetStatic(bool IsStatic);
	bool IsStatic() const { return bIsStatic; }
	void MarkCheckedThisFrame(AActor* OtherActor);
	void FlushOverlapState();

	virtual void RegisterComponent() override;
	virtual void UnRegisterComponent() override;
protected:
	void OnComponentDestroy() override;
private:
	std::unordered_set<AActor*> m_CheckedThisFrame;
	std::unordered_set<AActor*> m_OverlappingActors;
	std::unordered_map<MCollisionComponent*, std::uint64_t> m_LastCheckedFrame;
	std::unordered_set<AActor*> m_IntersectingThisFrame;
	ECollisionType m_CollisionType = ECollisionType::Overlap;
	bool bIsStatic = true;
};
