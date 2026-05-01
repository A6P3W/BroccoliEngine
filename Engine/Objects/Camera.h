#pragma once
#include "Core/GameObject.h"
class ACameraObject : public AGameObject
{
public:
	void SetTarget(AGameObject* target);
	void OnUpdate(float DeltaTime) override;
	void SetCameraView();
	void TraceRotation();
private:
	AGameObject* m_target = nullptr;
};

