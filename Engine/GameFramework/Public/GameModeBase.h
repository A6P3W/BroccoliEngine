#pragma once
#include "Actor.h"

class AGameModeBase : public AActor
{
public:
	AGameModeBase();
	void OnUpdate(float DeltaTime) override;
protected:
	AActor* m_PlayerPawn;
	AActor* m_PlayerController;
private:

};

