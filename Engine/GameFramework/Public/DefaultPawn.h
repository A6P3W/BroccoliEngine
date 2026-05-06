#pragma once
#include "GameObject.h"
class ADefaultPawn : public AGameObject
{
public:
	ADefaultPawn(FVector2D location, FRotator rotation);
	void OnUpdate(float DeltaTime) override;
private:

};

