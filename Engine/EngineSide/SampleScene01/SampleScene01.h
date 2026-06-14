#pragma once
#include "GameModeBase.h"
class ASampleScene01 :public AGameModeBase
{
public:
	DEFINE_ACTOR_CLASS(ASampleScene01)
	ASampleScene01();
	void BeginPlay() override;
	void OnUpdate(float DeltaTime) override;
private:

};

