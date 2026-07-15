#pragma once
#include <string>

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UIWidgetComponent.h"

class BROCCOLI_ENGINE_API UITextComponent : public MUIWidgetComponent {
 public:
  UITextComponent();
  ~UITextComponent() override;

  void OnRegister() override;
  void SetText(const std::string& text);
  void SetColor(const FColor& color);
  void SetFontSize(int fontSize);

 private:
  void UpdateText();

  struct Impl;
  Impl* ImplPtr = nullptr;
};
