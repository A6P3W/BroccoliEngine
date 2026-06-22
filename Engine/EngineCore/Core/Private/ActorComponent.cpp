#include "SceneComponent.h"
#include "Actor.h"
#include "TimerManager.h"
#include "Log.h"
#include "World.h"

#include "HttpManager.h"
MActorComponent::~MActorComponent()
{
	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
		GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
	}
	HttpManager::GetInstance().CancelAllRequestsForObject(this);
}

bool MActorComponent::Update(float DeltaTime)
{
	if (IsPendingDestroy()) {
		return false;
	}
	OnUpdate(DeltaTime);
	return true;
}

void MActorComponent::DestroyComponent()
{
	if (bPendingDestroy) {
		return;
	}

	if (Owner != nullptr) {
		const auto* rootComponent = Owner->GetRootComponent();
		if (rootComponent != nullptr && static_cast<const MActorComponent*>(rootComponent) == this) {
			M_LOG("DestroyComponent ignored: attempted to destroy RootComponent");
			return;
		}
	}

	bPendingDestroy = true;

	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
		GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
	}

	HttpManager::GetInstance().CancelAllRequestsForObject(this);
	UnRegisterComponent();

	OnComponentDestroy();
}
