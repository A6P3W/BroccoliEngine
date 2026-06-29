#include "UITextComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"

UITextComponent::UITextComponent(const std::string& text, int color, int fontSize)
    : m_Text(text), m_Color(color), m_FontSize(fontSize) {}

void UITextComponent::RegisterComponent() {
  if (m_TextSprite != nullptr || GetOwner() == nullptr) {
    return;
  }

  // 描画空間をScreenにし、親(ボタン等)よりもZオーダーを1つ上に設定
  auto sprite = std::make_unique<MSpriteComponent>(GetFinalPriority() + 1, RenderSpace::Screen);
  m_TextSprite = sprite.get();
  m_TextSprite->SetParentComponent(this);

  // フォントハンドルを取得
  m_FontHandle = ResourceManager::GetInstance().GetFont(m_FontSize, 5);

  UpdateText();
  GetOwner()->AddComponent(std::move(sprite));
}

void UITextComponent::SetText(const std::string& text) {
  m_Text = text;
  UpdateText();
}

void UITextComponent::SetColor(int color) {
  m_Color = color;
  UpdateText();
}

void UITextComponent::UpdateText() {
  if (!m_TextSprite) return;

  // 文字列の描画ピクセル幅を取得
  int textWidth = GetDrawStringWidthToHandle(m_Text.c_str(), m_Text.length(), m_FontHandle);
  // 高さはフォントサイズを基準にする
  int textHeight = m_FontSize;

  // 文字の中心を原点にするため、幅と高さの半分だけ左上にずらす（自動センタリング）
  m_TextSprite->SetRelativeLocation({-textWidth * 0.5f, -textHeight * 0.5f});

  m_TextSprite->SubmitText(m_Text, m_Color, m_FontHandle);
}
