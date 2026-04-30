#pragma once
#include "Core/GameObject.h"
class AGridLine :
    public AGameObject
{
public:
	AGridLine(int LineWidth);
	void OnDraw() override;
private:
	int m_LineWidth;
	int m_LineColor ;
};

