#include "UIInputTextComponent.h"

#include <algorithm>
#include <cctype>
#include <functional>
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
const std::string ActionHintText = "インタラクトキーで入力/確定";

bool IsShiftPressed(const KeyboardDevice* Keyboard) {
  return Keyboard != nullptr &&
         (Keyboard->GetPressing(EKey::LeftShift) || Keyboard->GetPressing(EKey::RightShift));
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
  bool bJustBeganInput = false;
  bool bCaretVisible = true;
  float CaretBlinkTimer = 0.0f;

  float Width = 0.0f;
  float Height = 0.0f;
  float TextOffsetY = 0.0f;
  float ActionHintOffsetY = 3.0f;
  FColor NormalColor = FColor::Black;
  FColor HoveredColor = FColor::Black;
  FColor EditingColor = FColor::Black;
  FColor TextColor = FColor::White;
  FColor HintColor = FColor{160, 160, 160};
  int FontSize = 24;
  int FontWeight = ResourceManager::DefaultFontWeight;
  int FontHandle = -1;
  int HintFontHandle = -1;
  EButtonState CurrentState = EButtonState::Normal;
};
UIInputTextComponent::UIInputTextComponent() : ImplPtr(new Impl()) {
  SetAllowGamepadSubmit(false);
  SetWidgetSize({ImplPtr->Width, ImplPtr->Height});
  ImplPtr->NormalColor = FColor{80, 80, 80};
  ImplPtr->HoveredColor = FColor{120, 120, 120};
  ImplPtr->EditingColor = FColor{45, 105, 170};
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
  if (ImplPtr->BoxSprite == nullptr || ImplPtr->TextSprite == nullptr ||
      ImplPtr->BorderSprite == nullptr || ImplPtr->ActionHintSprite == nullptr) {
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

  ImplPtr->FontHandle =
      ResourceManager::GetInstance().GetFont(ImplPtr->FontSize, ImplPtr->FontWeight);
  ImplPtr->HintFontHandle =
      ResourceManager::GetInstance().GetFont(ActionHintFontSize, ImplPtr->FontWeight);
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

  if (ImplPtr->bJustBeganInput) {
    ImplPtr->bJustBeganInput = false;
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

void UIInputTextComponent::SetColors(
    const FColor& normalColor, const FColor& hoveredColor, const FColor& editingColor
) {
  ImplPtr->NormalColor = normalColor;
  ImplPtr->HoveredColor = hoveredColor;
  ImplPtr->EditingColor = editingColor;
  UpdateVisuals();
}

void UIInputTextComponent::SetTextColor(const FColor& color) {
  ImplPtr->TextColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetHintColor(const FColor& color) {
  ImplPtr->HintColor = color;
  UpdateVisuals();
}

void UIInputTextComponent::SetFontWeight(int Weight) {
  Weight = ResourceManager::NormalizeFontWeight(Weight);
  if (ImplPtr->FontWeight == Weight) return;

  ImplPtr->FontWeight = Weight;
  if (ImplPtr->TextSprite != nullptr) {
    ImplPtr->FontHandle =
        ResourceManager::GetInstance().GetFont(ImplPtr->FontSize, ImplPtr->FontWeight);
    ImplPtr->HintFontHandle =
        ResourceManager::GetInstance().GetFont(ActionHintFontSize, ImplPtr->FontWeight);
  }
  UpdateVisuals();
}

int UIInputTextComponent::GetFontWeight() const { return ImplPtr->FontWeight; }

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
  ImplPtr->bJustBeganInput = true;
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
  const auto* Keyboard = InputManager::GetInstance().GetDevice<KeyboardDevice>();
  if (Keyboard == nullptr) {
    return;
  }

  if (Keyboard->GetPressStart(EKey::Enter)) {
    CommitInput();
    return;
  }

  if (Keyboard->GetPressStart(EKey::Escape)) {
    CancelInput();
    return;
  }

  if (Keyboard->GetPressStart(EKey::Backspace)) {
    if (!ImplPtr->EditingText.empty()) {
      ImplPtr->EditingText.pop_back();
      if (ImplPtr->OnTextChanged) {
        ImplPtr->OnTextChanged(ImplPtr->EditingText);
      }
      UpdateVisuals();
    }
    return;
  }

  const bool bUpper = IsShiftPressed(Keyboard);
  const std::pair<EKey, char> LetterKeys[] = {
      {EKey::A, 'a'}, {EKey::B, 'b'}, {EKey::C, 'c'}, {EKey::D, 'd'}, {EKey::E, 'e'},
      {EKey::F, 'f'}, {EKey::G, 'g'}, {EKey::H, 'h'}, {EKey::I, 'i'}, {EKey::J, 'j'},
      {EKey::K, 'k'}, {EKey::L, 'l'}, {EKey::M, 'm'}, {EKey::N, 'n'}, {EKey::O, 'o'},
      {EKey::P, 'p'}, {EKey::Q, 'q'}, {EKey::R, 'r'}, {EKey::S, 's'}, {EKey::T, 't'},
      {EKey::U, 'u'}, {EKey::V, 'v'}, {EKey::W, 'w'}, {EKey::X, 'x'}, {EKey::Y, 'y'},
      {EKey::Z, 'z'},
  };

  for (const auto& [KeyCode, Character] : LetterKeys) {
    if (Keyboard->GetPressStart(KeyCode)) {
      AppendCharacter(static_cast<char>(bUpper ? std::toupper(Character) : Character));
      return;
    }
  }

  const std::pair<EKey, char> DigitKeys[] = {
      {EKey::Zero, '0'},
      {EKey::One, '1'},
      {EKey::Two, '2'},
      {EKey::Three, '3'},
      {EKey::Four, '4'},
      {EKey::Five, '5'},
      {EKey::Six, '6'},
      {EKey::Seven, '7'},
      {EKey::Eight, '8'},
      {EKey::Nine, '9'},
  };

  for (const auto& [KeyCode, Character] : DigitKeys) {
    if (Keyboard->GetPressStart(KeyCode)) {
      AppendCharacter(Character);
      return;
    }
  }
}

void UIInputTextComponent::AppendCharacter(char character) {
  if (!IsAlphaNumeric(character) ||
      static_cast<int>(ImplPtr->EditingText.size()) >= ImplPtr->MaxLength) {
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
    ImplPtr->BoxSprite->SetRelativeLocation(FVector2D::ZeroVector());
    ImplPtr->BoxSprite->SubmitBox(
        ImplPtr->Width, ImplPtr->Height, GetCurrentBackgroundColor(), true
    );
  }

  if (ImplPtr->BorderSprite != nullptr) {
    ImplPtr->BorderSprite->SetRelativeLocation(FVector2D::ZeroVector());
    if (ImplPtr->bIsEditing) {
      ImplPtr->BorderSprite->SubmitBox(ImplPtr->Width, ImplPtr->Height, FColor{0, 255, 0}, false);
    } else if (ImplPtr->CurrentState == EButtonState::Hovered) {
      ImplPtr->BorderSprite->SubmitBox(
          ImplPtr->Width, ImplPtr->Height, FColor{255, 255, 255}, false
      );
    } else {
      ImplPtr->BorderSprite->SubmitBox(
          ImplPtr->Width, ImplPtr->Height, FColor{255, 255, 255, 0}, false
      );
    }
  }

  if (ImplPtr->ActionHintSprite != nullptr) {
    if (ImplPtr->CurrentState == EButtonState::Hovered && !ImplPtr->bIsEditing) {
      const int HintWidth =
          ResourceManager::GetInstance().GetTextWidth(ActionHintText, ImplPtr->HintFontHandle);
      ImplPtr->ActionHintSprite->SetRelativeLocation(
          {-HintWidth * 0.5f, ImplPtr->Height * 0.5f + ImplPtr->ActionHintOffsetY}
      );
      ImplPtr->ActionHintSprite->SubmitText(
          ActionHintText, FColor{255, 230, 64}, ImplPtr->HintFontHandle
      );
    } else {
      ImplPtr->ActionHintSprite->SubmitText("", FColor{255, 230, 64, 0}, ImplPtr->HintFontHandle);
    }
  }

  if (ImplPtr->TextSprite == nullptr) {
    return;
  }

  const std::string DisplayText = GetDisplayText();
  const bool bShowHint =
      !ImplPtr->bIsEditing && ImplPtr->Text.empty() && !ImplPtr->HintText.empty();
  FColor Color = bShowHint ? ImplPtr->HintColor : ImplPtr->TextColor;
  const int TextWidth =
      ResourceManager::GetInstance().GetTextWidth(DisplayText, ImplPtr->FontHandle);

  const float LeftPadding = 16.0f;
  const float MaxTextWidth = (std::max)(0.0f, ImplPtr->Width - LeftPadding * 2.0f);
  const float TextX = -ImplPtr->Width * 0.5f + LeftPadding;
  const float TextY = -ImplPtr->FontSize * 0.5f + ImplPtr->TextOffsetY;
  ImplPtr->TextSprite->SetRelativeLocation({TextX, TextY});
  ImplPtr->TextSprite->SubmitText(
      DisplayText,
      (Color.A = static_cast<uint8_t>(TextWidth > MaxTextWidth ? 220 : 255), Color),
      ImplPtr->FontHandle
  );
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

FColor UIInputTextComponent::GetCurrentBackgroundColor() const {
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
