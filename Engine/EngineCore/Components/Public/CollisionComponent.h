#pragma once
#include "SceneComponent.h"
#include "Actor.h"
#include <unordered_set>
enum class ECollisionShape
{
	Circle,
};
class MCollisionComponent : public MSceneComponent
{
public:
	MCollisionComponent();
	virtual ~MCollisionComponent();
	virtual ECollisionShape GetShapeType() const = 0;

	bool IsOverlappingActor(AActor* OtherActor) const {
		return m_OverlappingActors.contains(OtherActor);
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

private:
	std::unordered_set<AActor*> m_OverlappingActors;
};

