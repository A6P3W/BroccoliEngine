#include "UIButtonComponent.h"

void MUIButtonComponent::OnUpdate(float DeltaTime)
{}

void MUIButtonComponent::SetState(EButtonState NewState)
{
	if (NewState == ButtonState)
		return;
	ButtonState = NewState;
	OnStateChanged(NewState);
}
