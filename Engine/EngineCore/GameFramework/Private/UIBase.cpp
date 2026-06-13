#include "UIBase.h"
#include "EnhancedInputComponent.h"
#include "UIButtonComponent.h"
void AUIBase::Navigate(const FInputActionValue& Value)
{}

void AUIBase::Submit()
{
	if (FocusedButtonComponent)
	{
		FocusedButtonComponent->OnPressed();
	}
}

void AUIBase::Cancel()
{}
