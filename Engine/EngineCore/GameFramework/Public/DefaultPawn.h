#pragma once
#include "Actor.h"
class ADefaultPawn : public AActor
{
public:
	ADefaultPawn(FVector2D location, FRotator rotation);
	void OnUpdate(float DeltaTime) override;
private:

};

