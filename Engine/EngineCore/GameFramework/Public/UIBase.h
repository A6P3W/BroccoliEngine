#pragma once
#include "Actor.h"

struct FInputActionValue;

class MUIButtonComponent;
class AUIBase : public AActor
{
public:
	virtual void OnOpened() {}
	virtual void OnClosed() {}

	virtual void OnObscured() {}
	virtual void OnRevealed() {}

	void Navigate(const FInputActionValue& Value);
	void Submit();
	void Cancel();

private:
	MUIButtonComponent* FocusedButtonComponent = nullptr;
};

