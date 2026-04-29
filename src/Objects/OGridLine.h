#pragma once
#include "Core/OGameObject.h"
class OGridLine :
    public OGameObject
{
public:
	OGridLine(int LineWidth);
	void OnDraw() override;
private:
	int m_LineWidth;
};

