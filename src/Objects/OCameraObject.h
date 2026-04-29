#pragma once
#include "Core/OGameObject.h"
class OCameraObject : public OGameObject
{
public:
	void SetTarget(OGameObject* target);
	void OnUpdate() override;
	void SetCameraView();
	void TraceRotation();
private:
	OGameObject* m_target = nullptr;
};

