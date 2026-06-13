#pragma once
#include "UMath.h"
#include <vector>
struct FInputActionValue;
class AWidgetBase;
class UIManager
{
public :
	static UIManager* GetInstance() {
		static UIManager instance;
		return &instance;
	}

	void PushWidget(AWidgetBase* Widget);
	void PopWidget();
	AWidgetBase* GetPeekWidget();

	void Navigate(const FInputActionValue& Value);
	void Submit();
	void Cancel();

	void SetFocusedWidget(AWidgetBase* Widget) { CurrentFocusedWidget = Widget; }

private:
	AWidgetBase* CurrentFocusedWidget = nullptr;
	std::vector<AWidgetBase*> WidgetStack;
};

