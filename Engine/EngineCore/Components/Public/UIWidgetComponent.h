#pragma once
#include "SceneComponent.h"

class MUIWidgetComponent : public MSceneComponent
{
public:
	MUIWidgetComponent(int basePriority = 0);

	void SetZOrderOffset(int offset);

	int GetFinalPriority() const;

protected:
	int m_basePriority = 0;
	int m_zOrderOffset = 0;
};
