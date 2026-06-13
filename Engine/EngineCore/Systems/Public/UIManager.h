#pragma once
#include "UMath.h"
#include <vector>
struct FInputActionValue;
class UIBase;
class UIManager
{
public :
	static UIManager* GetInstance() {
		static UIManager instance;
		return &instance;
	}

	void PushWidget(UIBase* Widget);
	void PopWidget();
	UIBase* GetPeekWidget();

	void Navigate(const FInputActionValue& Value);
	void Submit();
	void Cancel();

	void SetFocusedWidget(UIBase* Widget) { CurrentFocusedWidget = Widget; }

private:
	UIBase* CurrentFocusedWidget = nullptr;
	std::vector<UIBase*> WidgetStack;
};

