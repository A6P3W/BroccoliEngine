#pragma once
#include <string>

#include "UIWidgetComponent.h"

class MSpriteComponent;

class UITextComponent : public MUIWidgetComponent {
 public:
  // デフォルトで白文字、サイズ24
  UITextComponent();

  void OnRegister() override;
  void SetText(const std::string& text);
  void SetColor(int color);
  void SetFontSize(int fontSize);

 private:
  void UpdateText();

  MSpriteComponent* TextSprite = nullptr;
  std::string Text;
  int Color = 0xFFFFFF;
  int FontSize = 24;
  int FontHandle = -1;
};
