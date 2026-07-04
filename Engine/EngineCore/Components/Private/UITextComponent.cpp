#include "UITextComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

UITextComponent::UITextComponent(const std::string& text, int color, int fontSize)
    : Text(text), Color(color), FontSize(fontSize) {}

void UITextComponent::RegisterComponent() {
  if (TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  // 描画空間をScreenにし、親(ボタン等)よりもZオーダーを1つ上に設定
  auto sprite = std::make_unique<MSpriteComponent>(GetFinalPriority() + 1, RenderSpace::Screen);
  TextSprite = sprite.get();
  TextSprite->SetParentComponent(this);

  // フォントハンドルを取得
  FontHandle = ResourceManager::GetInstance().GetFont(FontSize, 5);

  UpdateText();
  GetOwner()->AddComponent(std::move(sprite));
}

void UITextComponent::SetText(const std::string& text) {
  Text = text;
  UpdateText();
}

void UITextComponent::SetColor(int color) {
  Color = color;
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
