#pragma once
#include "Actor.h"
class AGridLine :
    public AActor
{
public:
	AGridLine();
	void OnUpdate(float DeltaTime) override;
	void SetLineWidth(float LineWidth);
private:
	float m_LineWidth;
	int m_LineColor ;
};

