#pragma once
#include "Core/Public/GameObject.h"
class AGridLine :
    public AGameObject
{
public:
	AGridLine();
	void OnUpdate(float DeltaTime) override;
	void SetLineWidth(float LineWidth);
private:
	float m_LineWidth;
	int m_LineColor ;
};

