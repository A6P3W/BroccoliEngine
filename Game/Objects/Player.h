#pragma once
#include "Core/Public/GameObject.h"
#include <Utils/Umath.h>
class APlayer : public AGameObject
{
public:
	APlayer(FVector2D location ,FRotator rotation);
	void OnUpdate(float DeltaTime) override;
	void OnDraw() override;
private:

};

