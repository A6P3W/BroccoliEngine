#pragma once
#include "GameModeBase.h"
class ADefaultScene :public AGameModeBase
{
public:
	ADefaultScene();
	void OnUpdate(float DeltaTime) override;
private:

};

