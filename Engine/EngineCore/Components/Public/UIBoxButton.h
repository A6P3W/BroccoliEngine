#pragma once
#include "UIButtonComponent.h"

class MSpriteComponent;

class UIBoxButtonComponent : public MUIButtonComponent
{
public:
	UIBoxButtonComponent(float width, float height, int normalColor, int hoveredColor, int pressedColor);

	void RegisterComponent() override;
	void OnStateChanged(EButtonState NewState) override;

	void SetSize(float width, float height);
	void SetColors(int normalColor, int hoveredColor, int pressedColor);

private:
	void UpdateBox();
	int GetColorForState(EButtonState State) const;

	MSpriteComponent* m_BoxSprite = nullptr;
	float m_Width = 0.0f;
	float m_Height = 0.0f;
	int m_NormalColor = 0;
	int m_HoveredColor = 0;
	int m_PressedColor = 0;
	EButtonState m_CurrentState = EButtonState::Normal;
};
