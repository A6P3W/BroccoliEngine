#pragma once
#include "SceneComponent.h"

class MCameraComponent : public MSceneComponent
{
public:
	~MCameraComponent();
	float GetFOV() const { return m_fov; }
	void SetFOV(float fov) { m_fov = fov; }
	void SetActiveCamera();
private:
	float m_fov=1.0f;
};

