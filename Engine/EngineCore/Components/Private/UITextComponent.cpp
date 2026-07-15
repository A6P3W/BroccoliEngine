#include "UITextComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

struct UITextComponent::Impl {
  MSpriteComponent* TextSprite = nullptr;
  std::string Text;
  FColor Color = FColor::White;
  int FontSize = 24;
  int FontHandle = -1;
};

UITextComponent::UITextComponent() : ImplPtr(new Impl()) {}

UITextComponent::~UITextComponent() { delete ImplPtr; }

void UITextComponent::OnRegister() {
  if (ImplPtr->TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  // 描画空間をScreenにし、親(ボタン等)よりもZオーダーを1つ上に設定
  ImplPtr->TextSprite = NewObject<MSpriteComponent>(GetOwner());
  if (ImplPtr->TextSprite == nullptr) {
    return;
  }
  ImplPtr->TextSprite->SetRenderSettings(GetFinalPriority() + 1, RenderSpace::Screen);
  ImplPtr->TextSprite->AttachToComponent(this);

  // フォントハンドルを取得
  ImplPtr->FontHandle = ResourceManager::GetInstance().GetFont(ImplPtr->FontSize, 5);

  UpdateText();
  ImplPtr->TextSprite->RegisterComponent();
}

void UITextComponent::SetText(const std::string& text) {
  ImplPtr->Text = text;
  UpdateText();
}

void UITextComponent::SetColor(const FColor& color) {
  ImplPtr->Color = color;
  UpdateText();
}

void UITextComponent::SetFontSize(int fontSize) {
  ImplPtr->FontSize = fontSize;
  if (ImplPtr->TextSprite != nullptr) {
    ImplPtr->FontHandle = ResourceManager::GetInstance().GetFont(ImplPtr->FontSize, 5);
  }
  UpdateText();
}

void UITextComponent::UpdateText() {
  if (!ImplPtr->TextSprite) return;

  // 文字列の描画ピクセル幅を取得
  int textWidth = GetDrawStringWidthToHandle(
      ImplPtr->Text.c_str(), static_cast<int>(ImplPtr->Text.length()), ImplPtr->FontHandle
  );
  // 高さはフォントサイズを基準にする
  int textHeight = ImplPtr->FontSize;

  // 文字の中心を原点にするため、幅と高さの半分だけ左上にずらす（自動センタリング）
  ImplPtr->TextSprite->SetRelativeLocation({-textWidth * 0.5f, -textHeight * 0.5f});

  ImplPtr->TextSprite->SubmitText(ImplPtr->Text, ImplPtr->Color, ImplPtr->FontHandle);
}
