#pragma once
#include "Core/GameObject.h"

class APlayer : public AGameObject
{
public:
	APlayer(float x,float y);
	void OnUpdate(float DeltaTime) override;
	void OnDraw() override;
private:

};

