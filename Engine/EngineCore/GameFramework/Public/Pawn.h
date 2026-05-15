#pragma once
#include "Actor.h"
#include <Utils/Umath.h>
class MCameraComponent;
class APawn : public AActor
{
public:
	APawn();
	virtual void OnPossesed();
	void OnUpdate(float DeltaTime) override;

	virtual void FowardBack(float Scale);
	virtual void LeftRight(float Scale);
private:
protected:
	MCameraComponent* m_camera = nullptr;
};

