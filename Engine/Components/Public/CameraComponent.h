#pragma once
#include "Components/Public/SceneComponent.h"

class MCameraComponent : public MSceneComponent
{
public:
	float GetFOV() const { return m_fov; }
	void SetActiveCamera();
private:
	float m_fov=90;
};

