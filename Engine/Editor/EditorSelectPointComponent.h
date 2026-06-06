#pragma once
#include "SceneComponent.h"

class MSpriteComponent;
class EditorSelectPointComponent : public MSceneComponent
{
public:
	EditorSelectPointComponent();
	void Selected(bool bSelected);
private:
	void RegisterComponent() override;
	MSpriteComponent* SelectPointSprite;
};

