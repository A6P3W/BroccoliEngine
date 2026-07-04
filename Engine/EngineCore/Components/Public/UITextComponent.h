#pragma once
#include <string>

#include "UIWidgetComponent.h"

class MSpriteComponent;

class UITextComponent : public MUIWidgetComponent {
 public:
  // デフォルトで白文字、サイズ24
  UITextComponent(const std::string& text = "", int color = 0xFFFFFF, int fontSize = 24);

  void RegisterComponent() override;
  void SetText(const std::string& text);
  void SetColor(int color);

 private:
  void UpdateText();

  MSpriteComponent* TextSprite = nullptr;
  std::string Text;
  int Color = 0xFFFFFF;
  int FontSize = 24;
  int FontHandle = -1;
};
