#pragma once

#include <functional>
#include <string>

#include "UIButtonComponent.h"

class MSpriteComponent;

class UIInputTextComponent : public MUIButtonComponent {
 public:
  UIInputTextComponent(float width, float height, const std::string& hintText = "");

  void RegisterComponent() override;
  void Press() override;
  void OnUpdate(float DeltaTime) override;
  void OnStateChanged(EButtonState NewState) override;

  void SetText(const std::string& text, bool bBroadcast = false);
  const std::string& GetText() const { return Text; }

  void SetHintText(const std::string& hintText);
  void SetMaxLength(int maxLength);
  void SetPassword(bool bInIsPassword);
  void SetSize(float width, float height);
  void SetColors(int normalColor, int hoveredColor, int editingColor);
  void SetTextColor(int color);
  void SetHintColor(int color);

  bool IsEditing() const { return bIsEditing; }

  std::function<void(const std::string&)> OnTextChanged;
  std::function<void(const std::string&)> OnTextCommitted;

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
  int GetCurrentBackgroundColor() const;

  MSpriteComponent* BoxSprite = nullptr;
  MSpriteComponent* TextSprite = nullptr;

  std::string Text;
  std::string HintText;
  std::string EditingText;
  int MaxLength = 16;
  bool bIsPassword = false;
  bool bIsEditing = false;
  bool bCaretVisible = true;
  float CaretBlinkTimer = 0.0f;

  float Width = 0.0f;
  float Height = 0.0f;
  int NormalColor = 0;
  int HoveredColor = 0;
  int EditingColor = 0;
  int TextColor = 0xFFFFFF;
  int HintColor = 0xA0A0A0;
  int FontSize = 24;
  int FontHandle = -1;
  EButtonState CurrentState = EButtonState::Normal;
};