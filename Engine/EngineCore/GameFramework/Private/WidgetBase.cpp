#include "WidgetBase.h"
#include "UIManager.h"
#include "EnhancedInputComponent.h"
#include "UIButtonComponent.h"
void AWidgetBase::Navigate(const FInputActionValue& Value)
{}

void AWidgetBase::Submit()
{
	if (FocusedButtonComponent)
	{
		FocusedButtonComponent->OnPressed();
	}
}

void AWidgetBase::Cancel()
{
	UIManager::GetInstance()->PopWidget();
}

void AWidgetBase::SetZOrderOffset(int offset)
{
	m_ZOrderOffset = offset;

	auto widgets = GetComponents<MUIWidgetComponent>();
	for (auto* widget : widgets)
	{
		widget->SetZOrderOffset(offset);
	}
}
