#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include <memory>

class AActor;

class MActorComponent : public MBaseObject {
public:
	virtual ~MActorComponent() override;
	virtual void BeginPlay() {}
	bool Update(float DeltaTime);
	virtual void Draw() {}

	void SetOwner(AActor* owner) { Owner = owner; }
	AActor* GetOwner() const { return Owner; }

	void DestroyComponent();
	bool IsPendingDestroy() const { return bPendingDestroy; }

	virtual void RegisterComponent(){}
	virtual void UnRegisterComponent() {}
protected:
	virtual void OnComponentDestroy() {}

	AActor* Owner = nullptr;
	virtual void OnUpdate(float DeltaTime) {}

private:
	bool bPendingDestroy = false;
};
