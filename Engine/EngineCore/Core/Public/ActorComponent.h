#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include <memory>

class AActor;

class MActorComponent : public MBaseObject {
public:
	bool Update(float DeltaTime);
	virtual void Draw() {}

	void SetOwner(AActor* owner) { m_owner = owner; }
	AActor* GetOwner() const { return m_owner; }

protected:
	AActor* m_owner = nullptr;
	virtual void OnUpdate(float DeltaTime) {}
};

