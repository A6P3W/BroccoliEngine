#pragma once
#include "Core/OGameObject.h"

class Player : public OGameObject
{
public:
	Player(float x,float y);
	void OnUpdate() override;
	void OnDraw() override;
private:

};

