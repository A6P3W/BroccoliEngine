#pragma once
#include "Actor.h"

struct FInputActionValue;
class MUIButtonComponent;

class AWidgetBase : public AActor
{
public:
	virtual ~AWidgetBase() override;

	void OnUpdate(float DeltaTime) override;

	virtual void OnOpened() {}
	virtual void OnClosed() {}
	virtual void OnObscured() {}
	virtual void OnRevealed() {}

	void SetFocusedButton(MUIButtonComponent* Button);
	void Navigate(const FInputActionValue& Value);
	void Submit();
	virtual void Cancel();

	void SetZOrderOffset(int offset);

private:
	MUIButtonComponent* FocusedButtonComponent = nullptr;
	int ZOrderOffset = 0;
	float NavigationCooldown = 0.0f;
};
