#pragma once
#include "GameObject.h"

class AGameModeBase : public AGameObject
{
public:
	AGameModeBase();
	void OnUpdate(float DeltaTime) override;
protected:
	AGameObject* m_PlayerPawn;
	AGameObject* m_PlayerController;
private:

};

