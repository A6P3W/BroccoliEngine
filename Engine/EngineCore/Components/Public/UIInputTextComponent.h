#pragma once
#include <functional>
#include <string>

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "UIButtonComponent.h"

class BROCCOLI_ENGINE_API UIInputTextComponent : public MUIButtonComponent {
 public:
  UIInputTextComponent();
  ~UIInputTextComponent() override;

  void OnRegister() override;
  void Press() override;
  void OnUpdate(float DeltaTime) override;
  void OnStateChanged(EButtonState NewState) override;

  void SetText(const std::string& text, bool bBroadcast = false);
  const std::string& GetText() const;

  void SetHintText(const std::string& hintText);
  void SetMaxLength(int maxLength);
  void SetPassword(bool bInIsPassword);
  void SetSize(float width, float height);
  void SetColors(const FColor& normalColor, const FColor& hoveredColor, const FColor& editingColor);
  void SetTextColor(const FColor& color);
  void SetHintColor(const FColor& color);
  void SetTextOffsetY(float offsetY);
  void SetActionHintOffsetY(float offsetY);
  void SetOnTextChanged(std::function<void(const std::string&)> Callback);
  void SetOnTextCommitted(std::function<void(const std::string&)> Callback);

  bool IsEditing() const;

 protected:
  void OnComponentDestroy() override;

 private:
  void BeginInput();
  void CommitInput();
  void CancelInput();
  void HandleKeyboardInput();
  void AppendCharacter(char character);
  void UpdateVisuals();
  std::string GetDisplayText() const;
  FColor GetCurrentBackgroundColor() const;

  struct Impl;
  Impl* ImplPtr = nullptr;
};
