#include "SceneComponent.h"
#include "Actor.h"
#include "TimerManager.h"
#include "Log.h"
#include "World.h"
MActorComponent::~MActorComponent()
{
	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
		GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
	}
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
	if (m_bPendingDestroy) {
		return;
	}

	if (m_owner != nullptr) {
		const auto* rootComponent = m_owner->GetRootComponent();
		if (rootComponent != nullptr && static_cast<const MActorComponent*>(rootComponent) == this) {
			M_LOG("DestroyComponent ignored: attempted to destroy RootComponent");
			return;
		}
	}

	m_bPendingDestroy = true;

	if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetTimerManager()) {
		GetOwner()->GetWorld()->GetTimerManager()->ClearAllTimersForObject(this);
	}

	UnRegisterComponent();

	OnComponentDestroy();
}
