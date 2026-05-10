#pragma once
#include "Actor.h"
#include <Utils/Umath.h>
class APlayer : public AActor
{
public:
	APlayer(FVector2D location ,FRotator rotation);
	void OnUpdate(float DeltaTime) override;
	void BeginOverlap(AActor* OtherActor) override;
private:

};

