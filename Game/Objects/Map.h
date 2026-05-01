#pragma once
#include <Core/GameObject.h>
class AMap : public AGameObject
{
public:
	AMap(float x, float y);
	void OnDraw() override;
};

