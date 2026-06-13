#pragma once
#include "Actor.h"

struct FInputActionValue;
class MUIButtonComponent;

class AWidgetBase : public AActor
{
public:
	virtual void OnOpened() {}
	virtual void OnClosed() {}
	virtual void OnObscured() {}
	virtual void OnRevealed() {}

	void Navigate(const FInputActionValue& Value);
	void Submit();
	void Cancel();

	void SetZOrderOffset(int offset);

private:
	MUIButtonComponent* FocusedButtonComponent = nullptr;
	int m_ZOrderOffset = 0;
};
