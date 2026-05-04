#pragma once
#include "Core/Public/GameObject.h"
class AGridLine :
    public AGameObject
{
public:
	AGridLine();
	void OnDraw() override;
	void SetLineWidth(float LineWidth);
private:
	int m_LineWidth;
	int m_LineColor ;
};

