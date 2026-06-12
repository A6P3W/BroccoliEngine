#pragma once
#include <functional>
#include "UIWidgetComponent.h"
class MUIButtonComponent : public MUIWidgetComponent
{
public:
	std::function<void()> OnPressed;
};

