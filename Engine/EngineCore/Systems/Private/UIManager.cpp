#include "UIManager.h"
#include "EnhancedInputComponent.h"

#include "Log.h"
#include "Actor.h"
#include "WidgetBase.h"
void UIManager::PushWidget(AWidgetBase* Widget)
{
	if (!WidgetStack.empty()) {
		WidgetStack.back()->OnObscured();
	}

	WidgetStack.push_back(Widget);

	Widget->OnOpened();
}
void UIManager::PopWidget()
{
	if (!WidgetStack.empty())
	{
		auto Widget = WidgetStack.back();
		Widget->OnClosed();
		Widget->Destroy();
		WidgetStack.pop_back();

		if (!WidgetStack.empty()) {
			WidgetStack.back()->OnRevealed();
		}
	}
}
AWidgetBase* UIManager::GetPeekWidget()
{
	if (!WidgetStack.empty())
	{
		return WidgetStack.back();
	}
	return nullptr;
}
void UIManager::Navigate(const FInputActionValue& Value)
{
	if (AWidgetBase* TopWidget = GetPeekWidget()) {
		TopWidget->Navigate(Value);
	}
}

void UIManager::Submit()
{
	if (AWidgetBase* TopWidget = GetPeekWidget()) {
		TopWidget->Submit();
	}
}

void UIManager::Cancel()
{
	if (AWidgetBase* TopWidget = GetPeekWidget()) {
		TopWidget->Cancel();
	}
}
