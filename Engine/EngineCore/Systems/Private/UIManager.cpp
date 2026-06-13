#include "UIManager.h"
#include "EnhancedInputComponent.h"

#include "Log.h"
void UIManager::PushWidget(UIBase* Widget)
{
	WidgetStack.push_back(Widget);
}
void UIManager::PopWidget()
{
	if (!WidgetStack.empty())
	{
		WidgetStack.pop_back();
	}
}
UIBase* UIManager::GetPeekWidget()
{
	if (!WidgetStack.empty())
	{
		return WidgetStack.back();
	}
	return nullptr;
}
void UIManager::Navigate(const FInputActionValue& Value)
{

}

void UIManager::Submit()
{}

void UIManager::Cancel()
{}
