#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include <memory>

class AActor;

class MActorComponent : public MBaseObject {
public:
	virtual ~MActorComponent() override;

	bool Update(float DeltaTime);
	virtual void Draw() {}

	void SetOwner(AActor* owner) { m_owner = owner; }
	AActor* GetOwner() const { return m_owner; }

	void DestroyComponent();
	bool IsPendingDestroy() const { return m_bPendingDestroy; }

protected:
	virtual void OnComponentDestroy() {}

	AActor* m_owner = nullptr;
	virtual void OnUpdate(float DeltaTime) {}

private:
	bool m_bPendingDestroy = false;
};
