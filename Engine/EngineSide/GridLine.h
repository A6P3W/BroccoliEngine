#pragma once
#include "Actor.h"
class AGridLine :
    public AActor
{
public:
	AGridLine();
	void OnUpdate(float DeltaTime) override;
	void SimpleDraw(float cellSize, int color);
private:
	float m_CollisionCellSize;
};

