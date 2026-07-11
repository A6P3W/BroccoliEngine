#include "UIInputTextComponent.h"

#include <DxLib.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <utility>

#include "Actor.h"
#include "InputManager.h"
#include "KeyboardDevice.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "UIManager.h"

namespace {
constexpr int ActionHintFontSize = 14;
const std::string ActionHintText = "決定/インタラクトで入力";

bool IsShiftPressed(const KeyboardDevice* Keyboard) {
  return Keyboard != nullptr &&
         (Keyboard->GetPressing(KEY_INPUT_LSHIFT) || Keyboard->GetPressing(KEY_INPUT_RSHIFT));
}

bool IsAlphaNumeric(char Character) {
  return std::isalnum(static_cast<unsigned char>(Character)) != 0;
}
}  // namespace

UIInputTextComponent::UIInputTextComponent() {
  SetWidgetSize({Width, Height});
  NormalColor = GetColor(80, 80, 80);
  HoveredColor = GetColor(120, 120, 120);
  EditingColor = GetColor(45, 105, 170);
}

void UIInputTextComponent::OnRegister() {
  if (BoxSprite != nullptr || TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  BoxSprite = NewObject<MSpriteComponent>(GetOwner());
  TextSprite = NewObject<MSpriteComponent>(GetOwner());
  BorderSprite = NewObject<MSpriteComponent>(GetOwner());
  ActionHintSprite = NewObject<MSpriteComponent>(GetOwner());
  if (BoxSprite == nullptr || TextSprite == nullptr || BorderSprite == nullptr ||
      ActionHintSprite == nullptr) {
    return;
  }

  BoxSprite->SetRenderSettings(GetFinalPriority(), RenderSpace::Screen);
  BoxSprite->SetParentComponent(this);
  TextSprite->SetRenderSettings(GetFinalPriority() + 1, RenderSpace::Screen);
  TextSprite->SetParentComponent(this);
  BorderSprite->SetRenderSettings(GetFinalPriority() + 2, RenderSpace::Screen);
  BorderSprite->SetParentComponent(this);
  ActionHintSprite->SetRenderSettings(GetFinalPriority() + 3, RenderSpace::Screen);
  ActionHintSprite->SetParentComponent(this);

  BoxSprite->RegisterComponent();
  TextSprite->RegisterComponent();
  BorderSprite->RegisterComponent();
  ActionHintSprite->RegisterComponent();

  FontHandle = ResourceManager::GetInstance().GetFont(FontSize, 5);
  HintFontHandle = ResourceManager::GetInstance().GetFont(ActionHintFontSize, 5);
  UpdateVisuals();
}

void UIInputTextComponent::Press() {
  if (CurrentState == EButtonState::Disabled) {
    return;
  }

  if (!bIsEditing) {
    BeginInput();
  }

  MUIButtonComponent::Press();
}

void UIInputTextComponent::OnUpdate(float DeltaTime) {
  MUIButtonComponent::OnUpdate(DeltaTime);

  if (!bIsEditing) {
    return;
  }

  CaretBlinkTimer += DeltaTime;
  if (CaretBlinkTimer >= 0.5f) {
    CaretBlinkTimer = 0.0f;
    bCaretVisible = !bCaretVisible;
    UpdateVisuals();
  }

  HandleKeyboardInput();
}

void UIInputTextComponent::OnStateChanged(EButtonState NewState) {
  CurrentState = NewState;
  UpdateVisuals();
}

void UIInputTextComponent::SetText(const std::string& text, bool bBroadcast) {
  Text = text.substr(0, static_cast<size_t>((std::max)(0, MaxLength)));
  if (bIsEditing) {
    EditingText = Text;
  }

  UpdateVisuals();

  if (bBroadcast && OnTextChanged) {
    OnTextChanged(Text);
  }
}

void UIInputTextComponent::SetHintText(const std::string& hintText) {
  HintText = hintText;
  UpdateVisuals();
}

void UIInputTextComponent::SetMaxLength(int maxLength) {
  MaxLength = (std::max)(0, maxLength);
  if (static_cast<int>(Text.size()) > MaxLength) {
    Text.resize(MaxLength);
  }
  if (static_cast<int>(EditingText.size()) > MaxLength) {
    EditingText.resize(MaxLength);
  }
  UpdateVisuals();
}

void UIInputTextComponent::SetPassword(bool bInIsPassword) {
  bIsPassword = bInIsPassword;
  UpdateVisuals();
}

void UIInputTextComponent::SetSize(float width, float height) {
  Width = width;
  Height = height;
  SetWidgetSize({width, height});
  UpdateVisuals();
}

void UIInputTextComponent::SetColors(int normalColor, int hoveredColor, int editingColor) {
  NormalColor = normalColor;
  HoveredColor = hoveredColor;
  EditingColor = editingColor;
  UpdateVisuals();
}

void UIInputTextComponent::SetTextColor(int color) {
  TextColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetHintColor(int color) {
  HintColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetTextOffsetY(float offsetY) {
  TextOffsetY = offsetY;
  UpdateVisuals();
}

void UIInputTextComponent::SetActionHintOffsetY(float offsetY) {
  ActionHintOffsetY = offsetY;
  UpdateVisuals();
}

void UIInputTextComponent::OnComponentDestroy() {
  if (bIsEditing) {
    UIManager::GetInstance()->SetTextInputActive(false);
  }

  MUIButtonComponent::OnComponentDestroy();
}

void UIInputTextComponent::BeginInput() {
  bIsEditing = true;
  EditingText = Text;
  CaretBlinkTimer = 0.0f;
  bCaretVisible = true;
  UIManager::GetInstance()->SetTextInputActive(true);
  SetState(EButtonState::Pressed);
  UpdateVisuals();
}

void UIInputTextComponent::CommitInput() {
  if (!bIsEditing) {
    return;
  }

  bIsEditing = false;
  UIManager::GetInstance()->SetTextInputActive(false);
  Text = EditingText;

  if (OnTextCommitted) {
    OnTextCommitted(Text);
  }

  SetState(EButtonState::Hovered);
  UpdateVisuals();
}

void UIInputTextComponent::CancelInput() {
  if (!bIsEditing) {
    return;
  }

  bIsEditing = false;
  UIManager::GetInstance()->SetTextInputActive(false);
  EditingText = Text;
  SetState(EButtonState::Hovered);
  UpdateVisuals();
}

void UIInputTextComponent::HandleKeyboardInput() {
  const auto* keyboard = InputManager::GetInstance().GetDevice<KeyboardDevice>();
  if (keyboard == nullptr) {
    return;
  }

  if (keyboard->GetPressStart(KEY_INPUT_RETURN)) {
    CommitInput();
    return;
  }

  if (keyboard->GetPressStart(KEY_INPUT_ESCAPE)) {
    CancelInput();
    return;
  }

  if (keyboard->GetPressStart(KEY_INPUT_BACK)) {
    if (!EditingText.empty()) {
      EditingText.pop_back();
      if (OnTextChanged) {
        OnTextChanged(EditingText);
      }
      UpdateVisuals();
    }
    return;
  }

  const bool bUpper = IsShiftPressed(keyboard);
  const std::pair<int, char> letterKeys[] = {
      {KEY_INPUT_A, 'a'}, {KEY_INPUT_B, 'b'}, {KEY_INPUT_C, 'c'}, {KEY_INPUT_D, 'd'},
      {KEY_INPUT_E, 'e'}, {KEY_INPUT_F, 'f'}, {KEY_INPUT_G, 'g'}, {KEY_INPUT_H, 'h'},
      {KEY_INPUT_I, 'i'}, {KEY_INPUT_J, 'j'}, {KEY_INPUT_K, 'k'}, {KEY_INPUT_L, 'l'},
      {KEY_INPUT_M, 'm'}, {KEY_INPUT_N, 'n'}, {KEY_INPUT_O, 'o'}, {KEY_INPUT_P, 'p'},
      {KEY_INPUT_Q, 'q'}, {KEY_INPUT_R, 'r'}, {KEY_INPUT_S, 's'}, {KEY_INPUT_T, 't'},
      {KEY_INPUT_U, 'u'}, {KEY_INPUT_V, 'v'}, {KEY_INPUT_W, 'w'}, {KEY_INPUT_X, 'x'},
      {KEY_INPUT_Y, 'y'}, {KEY_INPUT_Z, 'z'},
  };

  for (const auto& [keyCode, character] : letterKeys) {
    if (keyboard->GetPressStart(keyCode)) {
      AppendCharacter(static_cast<char>(bUpper ? std::toupper(character) : character));
      return;
    }
  }

  const std::pair<int, char> digitKeys[] = {
      {KEY_INPUT_0, '0'}, {KEY_INPUT_1, '1'}, {KEY_INPUT_2, '2'}, {KEY_INPUT_3, '3'},
      {KEY_INPUT_4, '4'}, {KEY_INPUT_5, '5'}, {KEY_INPUT_6, '6'}, {KEY_INPUT_7, '7'},
      {KEY_INPUT_8, '8'}, {KEY_INPUT_9, '9'},
  };

  for (const auto& [keyCode, character] : digitKeys) {
    if (keyboard->GetPressStart(keyCode)) {
      AppendCharacter(character);
      return;
    }
  }
}

void UIInputTextComponent::AppendCharacter(char character) {
  if (!IsAlphaNumeric(character) || static_cast<int>(EditingText.size()) >= MaxLength) {
    return;
  }

  EditingText.push_back(character);
  CaretBlinkTimer = 0.0f;
  bCaretVisible = true;

  if (OnTextChanged) {
    OnTextChanged(EditingText);
  }

  UpdateVisuals();
}

void UIInputTextComponent::UpdateVisuals() {
  if (BoxSprite != nullptr) {
    BoxSprite->SetRelativeLocation({-Width * 0.5f, -Height * 0.5f});
    BoxSprite->SubmitBox(Width, Height, GetCurrentBackgroundColor(), true);
  }

  if (BorderSprite != nullptr) {
    BorderSprite->SetRelativeLocation({-Width * 0.5f, -Height * 0.5f});
    if (bIsEditing) {
      BorderSprite->SubmitBox(Width, Height, GetColor(0, 255, 0), false);
    } else if (CurrentState == EButtonState::Hovered) {
      BorderSprite->SubmitBox(Width, Height, GetColor(255, 255, 255), false);
    } else {
      BorderSprite->SubmitBox(Width, Height, GetColor(255, 255, 255), false, 0);
    }
  }

  if (ActionHintSprite != nullptr) {
    if (CurrentState == EButtonState::Hovered && !bIsEditing) {
      const int hintWidth = GetDrawStringWidthToHandle(
          ActionHintText.c_str(), static_cast<int>(ActionHintText.length()), HintFontHandle
      );
      ActionHintSprite->SetRelativeLocation({-hintWidth * 0.5f, Height * 0.5f + ActionHintOffsetY});
      ActionHintSprite->SubmitText(ActionHintText, GetColor(255, 230, 64), HintFontHandle);
    } else {
      ActionHintSprite->SubmitText("", GetColor(255, 230, 64), HintFontHandle, 0);
    }
  }

  if (TextSprite == nullptr) {
    return;
  }

  const std::string displayText = GetDisplayText();
  const bool bShowHint = !bIsEditing && Text.empty() && !HintText.empty();
  const int color = bShowHint ? HintColor : TextColor;
  const int textWidth =
      GetDrawStringWidthToHandle(displayText.c_str(), static_cast<int>(displayText.length()), FontHandle);

  const float leftPadding = 16.0f;
  const float maxTextWidth = (std::max)(0.0f, Width - leftPadding * 2.0f);
  const float textX = -Width * 0.5f + leftPadding;
  const float textY = -FontSize * 0.5f + TextOffsetY;
  TextSprite->SetRelativeLocation({textX, textY});
  TextSprite->SubmitText(displayText, color, FontHandle, textWidth > maxTextWidth ? 220 : 255);
}

std::string UIInputTextComponent::GetDisplayText() const {
  const std::string& source = bIsEditing ? EditingText : Text;
  std::string result;

  if (source.empty() && !bIsEditing) {
    result = HintText;
  } else if (bIsPassword) {
    result.assign(source.size(), '*');
  } else {
    result = source;
  }

  if (bIsEditing && bCaretVisible) {
    result.push_back('|');
  }

  return result;
}

int UIInputTextComponent::GetCurrentBackgroundColor() const {
  if (bIsEditing) {
    return EditingColor;
  }

  switch (CurrentState) {
    case EButtonState::Hovered:
      return HoveredColor;
    case EButtonState::Pressed:
      return EditingColor;
    case EButtonState::Normal:
    case EButtonState::Disabled:
    default:
      return NormalColor;
  }
}
