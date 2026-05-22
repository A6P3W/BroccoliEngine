#pragma once
#include "Actor.h"

class AGameModeBase : public AActor
{
public:
	AGameModeBase();
protected:
	AActor* m_PlayerPawn;
	AActor* m_PlayerController;
private:

};

