#include "UITextComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

UITextComponent::UITextComponent() {}

void UITextComponent::OnRegister() {
  if (TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  // 描画空間をScreenにし、親(ボタン等)よりもZオーダーを1つ上に設定
  TextSprite = NewObject<MSpriteComponent>(GetOwner());
  if (TextSprite == nullptr) {
    return;
  }
  TextSprite->SetRenderSettings(GetFinalPriority() + 1, RenderSpace::Screen);
  TextSprite->AttachToComponent(this);

  // フォントハンドルを取得
  FontHandle = ResourceManager::GetInstance().GetFont(FontSize, 5);

  UpdateText();
  TextSprite->RegisterComponent();
}

void UITextComponent::SetText(const std::string& text) {
  Text = text;
  UpdateText();
}

void UITextComponent::SetColor(int color) {
  Color = color;
  UpdateText();
}

void UITextComponent::SetFontSize(int fontSize) {
  FontSize = fontSize;
  if (TextSprite != nullptr) {
    FontHandle = ResourceManager::GetInstance().GetFont(FontSize, 5);
  }
  UpdateText();
}

void UITextComponent::UpdateText() {
  if (!TextSprite) return;

  // 文字列の描画ピクセル幅を取得
  int textWidth = GetDrawStringWidthToHandle(Text.c_str(), Text.length(), FontHandle);
  // 高さはフォントサイズを基準にする
  int textHeight = FontSize;

  // 文字の中心を原点にするため、幅と高さの半分だけ左上にずらす（自動センタリング）
  TextSprite->SetRelativeLocation({-textWidth * 0.5f, -textHeight * 0.5f});

  TextSprite->SubmitText(Text, Color, FontHandle);
}
