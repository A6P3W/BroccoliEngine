#pragma once
#include "Core/OGameObject.h"

class Player : public OGameObject
{
public:
	Player(float x,float y);
	void OnUpdate(float DeltaTime) override;
	void OnDraw() override;
private:

};

