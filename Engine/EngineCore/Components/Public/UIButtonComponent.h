#pragma once
#include <functional>
#include "UIWidgetComponent.h"
enum class EButtonState
{
	Normal,
	Hovered,
	Pressed,
	Disabled
};

class MUIButtonComponent : public MUIWidgetComponent
{
public:
	struct FNavigationLinks
	{
		MUIButtonComponent* Up = nullptr;
		MUIButtonComponent* Down = nullptr;
		MUIButtonComponent* Left = nullptr;
		MUIButtonComponent* Right = nullptr;
	};

	void OnUpdate(float DeltaTime) override;
	std::function<void()> OnPressed;
	void SetState(EButtonState NewState) { ButtonState = NewState; }

	FNavigationLinks Navigation;

private:
	int Width = 100;
	int Height = 50;
	EButtonState ButtonState = EButtonState::Normal;
};

