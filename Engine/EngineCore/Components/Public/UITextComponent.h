#pragma once
#include "UIWidgetComponent.h"
#include <string>

class MSpriteComponent;

class UITextComponent : public MUIWidgetComponent
{
public:
	// デフォルトで白文字、サイズ24
	UITextComponent(const std::string& text = "", int color = 0xFFFFFF, int fontSize = 24);

	void RegisterComponent() override;
	void SetText(const std::string& text);
	void SetColor(int color);

private:
	void UpdateText();

	MSpriteComponent* m_TextSprite = nullptr;
	std::string m_Text;
	int m_Color = 0xFFFFFF;
	int m_FontSize = 24;
	int m_FontHandle = -1;
};
