#pragma once
#include "GameObject.h"
#include <Utils/Umath.h>
class AMap : public AGameObject
{
public:
	AMap(FVector2D location, FRotator rotation);
	void OnDraw() override;
};

