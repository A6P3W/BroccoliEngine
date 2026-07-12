#pragma once
#include "BroccoliEngineAPI.h"
#include <string>

#include "UIWidgetComponent.h"

class BROCCOLI_ENGINE_API UITextComponent : public MUIWidgetComponent {
 public:
  UITextComponent();
  ~UITextComponent() override;

  void OnRegister() override;
  void SetText(const std::string& text);
  void SetColor(int color);
  void SetFontSize(int fontSize);

 private:
  void UpdateText();

  struct Impl;
  Impl* ImplPtr = nullptr;
};