#include "UIInputTextComponent.h"

#include <DxLib.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <functional>
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

struct UIInputTextComponent::Impl {
  MSpriteComponent* BoxSprite = nullptr;
  MSpriteComponent* TextSprite = nullptr;
  MSpriteComponent* BorderSprite = nullptr;
  MSpriteComponent* ActionHintSprite = nullptr;

  std::string Text;
  std::string HintText;
  std::string EditingText;
  std::function<void(const std::string&)> OnTextChanged;
  std::function<void(const std::string&)> OnTextCommitted;
  int MaxLength = 16;
  bool bIsPassword = false;
  bool bIsEditing = false;
  bool bCaretVisible = true;
  float CaretBlinkTimer = 0.0f;

  float Width = 0.0f;
  float Height = 0.0f;
  float TextOffsetY = 0.0f;
  float ActionHintOffsetY = 3.0f;
  int NormalColor = 0;
  int HoveredColor = 0;
  int EditingColor = 0;
  int TextColor = 0xFFFFFF;
  int HintColor = 0xA0A0A0;
  int FontSize = 24;
  int FontHandle = -1;
  int HintFontHandle = -1;
  EButtonState CurrentState = EButtonState::Normal;
};
UIInputTextComponent::UIInputTextComponent() : ImplPtr(new Impl()) {
  SetWidgetSize({ImplPtr->Width, ImplPtr->Height});
  ImplPtr->NormalColor = GetColor(80, 80, 80);
  ImplPtr->HoveredColor = GetColor(120, 120, 120);
  ImplPtr->EditingColor = GetColor(45, 105, 170);
}

UIInputTextComponent::~UIInputTextComponent() { delete ImplPtr; }

const std::string& UIInputTextComponent::GetText() const { return ImplPtr->Text; }

bool UIInputTextComponent::IsEditing() const { return ImplPtr->bIsEditing; }

void UIInputTextComponent::OnRegister() {
  if (ImplPtr->BoxSprite != nullptr || ImplPtr->TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  ImplPtr->BoxSprite = NewObject<MSpriteComponent>(GetOwner());
  ImplPtr->TextSprite = NewObject<MSpriteComponent>(GetOwner());
  ImplPtr->BorderSprite = NewObject<MSpriteComponent>(GetOwner());
  ImplPtr->ActionHintSprite = NewObject<MSpriteComponent>(GetOwner());
  if (ImplPtr->BoxSprite == nullptr || ImplPtr->TextSprite == nullptr || ImplPtr->BorderSprite == nullptr ||
      ImplPtr->ActionHintSprite == nullptr) {
    return;
  }

  ImplPtr->BoxSprite->SetRenderSettings(GetFinalPriority(), RenderSpace::Screen);
  ImplPtr->BoxSprite->AttachToComponent(this);
  ImplPtr->TextSprite->SetRenderSettings(GetFinalPriority() + 1, RenderSpace::Screen);
  ImplPtr->TextSprite->AttachToComponent(this);
  ImplPtr->BorderSprite->SetRenderSettings(GetFinalPriority() + 2, RenderSpace::Screen);
  ImplPtr->BorderSprite->AttachToComponent(this);
  ImplPtr->ActionHintSprite->SetRenderSettings(GetFinalPriority() + 3, RenderSpace::Screen);
  ImplPtr->ActionHintSprite->AttachToComponent(this);

  ImplPtr->BoxSprite->RegisterComponent();
  ImplPtr->TextSprite->RegisterComponent();
  ImplPtr->BorderSprite->RegisterComponent();
  ImplPtr->ActionHintSprite->RegisterComponent();

  ImplPtr->FontHandle = ResourceManager::GetInstance().GetFont(ImplPtr->FontSize, 5);
  ImplPtr->HintFontHandle = ResourceManager::GetInstance().GetFont(ActionHintFontSize, 5);
  UpdateVisuals();
}

void UIInputTextComponent::Press() {
  if (ImplPtr->CurrentState == EButtonState::Disabled) {
    return;
  }

  if (!ImplPtr->bIsEditing) {
    BeginInput();
  }

  MUIButtonComponent::Press();
}

void UIInputTextComponent::OnUpdate(float DeltaTime) {
  MUIButtonComponent::OnUpdate(DeltaTime);

  if (!ImplPtr->bIsEditing) {
    return;
  }

  ImplPtr->CaretBlinkTimer += DeltaTime;
  if (ImplPtr->CaretBlinkTimer >= 0.5f) {
    ImplPtr->CaretBlinkTimer = 0.0f;
    ImplPtr->bCaretVisible = !ImplPtr->bCaretVisible;
    UpdateVisuals();
  }

  HandleKeyboardInput();
}

void UIInputTextComponent::OnStateChanged(EButtonState NewState) {
  ImplPtr->CurrentState = NewState;
  UpdateVisuals();
}

void UIInputTextComponent::SetText(const std::string& text, bool bBroadcast) {
  ImplPtr->Text = text.substr(0, static_cast<size_t>((std::max)(0, ImplPtr->MaxLength)));
  if (ImplPtr->bIsEditing) {
    ImplPtr->EditingText = ImplPtr->Text;
  }

  UpdateVisuals();

  if (bBroadcast && ImplPtr->OnTextChanged) {
    ImplPtr->OnTextChanged(ImplPtr->Text);
  }
}

void UIInputTextComponent::SetHintText(const std::string& hintText) {
  ImplPtr->HintText = hintText;
  UpdateVisuals();
}

void UIInputTextComponent::SetMaxLength(int maxLength) {
  ImplPtr->MaxLength = (std::max)(0, maxLength);
  if (static_cast<int>(ImplPtr->Text.size()) > ImplPtr->MaxLength) {
    ImplPtr->Text.resize(ImplPtr->MaxLength);
  }
  if (static_cast<int>(ImplPtr->EditingText.size()) > ImplPtr->MaxLength) {
    ImplPtr->EditingText.resize(ImplPtr->MaxLength);
  }
  UpdateVisuals();
}

void UIInputTextComponent::SetPassword(bool bInIsPassword) {
  ImplPtr->bIsPassword = bInIsPassword;
  UpdateVisuals();
}

void UIInputTextComponent::SetSize(float width, float height) {
  ImplPtr->Width = width;
  ImplPtr->Height = height;
  SetWidgetSize({width, height});
  UpdateVisuals();
}

void UIInputTextComponent::SetColors(int normalColor, int hoveredColor, int editingColor) {
  ImplPtr->NormalColor = normalColor;
  ImplPtr->HoveredColor = hoveredColor;
  ImplPtr->EditingColor = editingColor;
  UpdateVisuals();
}

void UIInputTextComponent::SetTextColor(int color) {
  ImplPtr->TextColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetHintColor(int color) {
  ImplPtr->HintColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetTextOffsetY(float offsetY) {
  ImplPtr->TextOffsetY = offsetY;
  UpdateVisuals();
}

void UIInputTextComponent::SetActionHintOffsetY(float offsetY) {
  ImplPtr->ActionHintOffsetY = offsetY;
  UpdateVisuals();
}

void UIInputTextComponent::SetOnTextChanged(std::function<void(const std::string&)> Callback) {
  ImplPtr->OnTextChanged = std::move(Callback);
}

void UIInputTextComponent::SetOnTextCommitted(std::function<void(const std::string&)> Callback) {
  ImplPtr->OnTextCommitted = std::move(Callback);
}
void UIInputTextComponent::OnComponentDestroy() {
  if (ImplPtr->bIsEditing) {
    UIManager::GetInstance()->SetTextInputActive(false);
  }

  MUIButtonComponent::OnComponentDestroy();
}

void UIInputTextComponent::BeginInput() {
  ImplPtr->bIsEditing = true;
  ImplPtr->EditingText = ImplPtr->Text;
  ImplPtr->CaretBlinkTimer = 0.0f;
  ImplPtr->bCaretVisible = true;
  UIManager::GetInstance()->SetTextInputActive(true);
  SetState(EButtonState::Pressed);
  UpdateVisuals();
}

void UIInputTextComponent::CommitInput() {
  if (!ImplPtr->bIsEditing) {
    return;
  }

  ImplPtr->bIsEditing = false;
  UIManager::GetInstance()->SetTextInputActive(false);
  ImplPtr->Text = ImplPtr->EditingText;

  if (ImplPtr->OnTextCommitted) {
    ImplPtr->OnTextCommitted(ImplPtr->Text);
  }

  SetState(EButtonState::Hovered);
  UpdateVisuals();
}

void UIInputTextComponent::CancelInput() {
  if (!ImplPtr->bIsEditing) {
    return;
  }

  ImplPtr->bIsEditing = false;
  UIManager::GetInstance()->SetTextInputActive(false);
  ImplPtr->EditingText = ImplPtr->Text;
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
    if (!ImplPtr->EditingText.empty()) {
      ImplPtr->EditingText.pop_back();
      if (ImplPtr->OnTextChanged) {
        ImplPtr->OnTextChanged(ImplPtr->EditingText);
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
  if (!IsAlphaNumeric(character) || static_cast<int>(ImplPtr->EditingText.size()) >= ImplPtr->MaxLength) {
    return;
  }

  ImplPtr->EditingText.push_back(character);
  ImplPtr->CaretBlinkTimer = 0.0f;
  ImplPtr->bCaretVisible = true;

  if (ImplPtr->OnTextChanged) {
    ImplPtr->OnTextChanged(ImplPtr->EditingText);
  }

  UpdateVisuals();
}

void UIInputTextComponent::UpdateVisuals() {
  if (ImplPtr->BoxSprite != nullptr) {
    ImplPtr->BoxSprite->SetRelativeLocation({-ImplPtr->Width * 0.5f, -ImplPtr->Height * 0.5f});
    ImplPtr->BoxSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, GetCurrentBackgroundColor(), true);
  }

  if (ImplPtr->BorderSprite != nullptr) {
    ImplPtr->BorderSprite->SetRelativeLocation({-ImplPtr->Width * 0.5f, -ImplPtr->Height * 0.5f});
    if (ImplPtr->bIsEditing) {
      ImplPtr->BorderSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, GetColor(0, 255, 0), false);
    } else if (ImplPtr->CurrentState == EButtonState::Hovered) {
      ImplPtr->BorderSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, GetColor(255, 255, 255), false);
    } else {
      ImplPtr->BorderSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, GetColor(255, 255, 255), false, 0);
    }
  }

  if (ImplPtr->ActionHintSprite != nullptr) {
    if (ImplPtr->CurrentState == EButtonState::Hovered && !ImplPtr->bIsEditing) {
      const int hintWidth = GetDrawStringWidthToHandle(
          ActionHintText.c_str(), static_cast<int>(ActionHintText.length()), ImplPtr->HintFontHandle
      );
      ImplPtr->ActionHintSprite->SetRelativeLocation({-hintWidth * 0.5f, ImplPtr->Height * 0.5f + ImplPtr->ActionHintOffsetY});
      ImplPtr->ActionHintSprite->SubmitText(ActionHintText, GetColor(255, 230, 64), ImplPtr->HintFontHandle);
    } else {
      ImplPtr->ActionHintSprite->SubmitText("", GetColor(255, 230, 64), ImplPtr->HintFontHandle, 0);
    }
  }

  if (ImplPtr->TextSprite == nullptr) {
    return;
  }

  const std::string displayText = GetDisplayText();
  const bool bShowHint = !ImplPtr->bIsEditing && ImplPtr->Text.empty() && !ImplPtr->HintText.empty();
  const int color = bShowHint ? ImplPtr->HintColor : ImplPtr->TextColor;
  const int textWidth =
      GetDrawStringWidthToHandle(displayText.c_str(), static_cast<int>(displayText.length()), ImplPtr->FontHandle);

  const float leftPadding = 16.0f;
  const float maxTextWidth = (std::max)(0.0f, ImplPtr->Width - leftPadding * 2.0f);
  const float textX = -ImplPtr->Width * 0.5f + leftPadding;
  const float textY = -ImplPtr->FontSize * 0.5f + ImplPtr->TextOffsetY;
  ImplPtr->TextSprite->SetRelativeLocation({textX, textY});
  ImplPtr->TextSprite->SubmitText(displayText, color, ImplPtr->FontHandle, textWidth > maxTextWidth ? 220 : 255);
}

std::string UIInputTextComponent::GetDisplayText() const {
  const std::string& source = ImplPtr->bIsEditing ? ImplPtr->EditingText : ImplPtr->Text;
  std::string result;

  if (source.empty() && !ImplPtr->bIsEditing) {
    result = ImplPtr->HintText;
  } else if (ImplPtr->bIsPassword) {
    result.assign(source.size(), '*');
  } else {
    result = source;
  }

  if (ImplPtr->bIsEditing && ImplPtr->bCaretVisible) {
    result.push_back('|');
  }

  return result;
}

int UIInputTextComponent::GetCurrentBackgroundColor() const {
  if (ImplPtr->bIsEditing) {
    return ImplPtr->EditingColor;
  }

  switch (ImplPtr->CurrentState) {
    case EButtonState::Hovered:
      return ImplPtr->HoveredColor;
    case EButtonState::Pressed:
      return ImplPtr->EditingColor;
    case EButtonState::Normal:
    case EButtonState::Disabled:
    default:
      return ImplPtr->NormalColor;
  }
}
