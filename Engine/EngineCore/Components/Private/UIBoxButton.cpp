#include "UIBoxButton.h"
#include "Actor.h"
#include "SpriteComponent.h"
#include <memory>

UIBoxButtonComponent::UIBoxButtonComponent(float width, float height, int normalColor, int hoveredColor, int pressedColor)
	: m_Width(width)
	, m_Height(height)
	, m_NormalColor(normalColor)
	, m_HoveredColor(hoveredColor)
	, m_PressedColor(pressedColor)
{
	SetWidgetSize({ width, height });
}

void UIBoxButtonComponent::RegisterComponent()
{
	if (m_BoxSprite != nullptr || GetOwner() == nullptr) {
		return;
	}
	auto boxSprite = std::make_unique<MSpriteComponent>(GetFinalPriority(), RenderSpace::Screen);
	m_BoxSprite = boxSprite.get();
	m_BoxSprite->SetParentComponent(this);
	UpdateBox();
	GetOwner()->AddComponent(std::move(boxSprite));
}

void UIBoxButtonComponent::OnStateChanged(EButtonState NewState)
{
	m_CurrentState = NewState;
	UpdateBox();
}

void UIBoxButtonComponent::SetSize(float width, float height)
{
	m_Width = width;
	m_Height = height;
	SetWidgetSize({ width, height });
	UpdateBox();
}

void UIBoxButtonComponent::SetColors(int normalColor, int hoveredColor, int pressedColor)
{
	m_NormalColor = normalColor;
	m_HoveredColor = hoveredColor;
	m_PressedColor = pressedColor;
	UpdateBox();
}

void UIBoxButtonComponent::UpdateBox()
{
	if (m_BoxSprite == nullptr) {
		return;
	}
	m_BoxSprite->SetRelativeLocation({ -m_Width * 0.5f, -m_Height * 0.5f });
	m_BoxSprite->SubmitBox(m_Width, m_Height, GetColorForState(m_CurrentState), true);
}

int UIBoxButtonComponent::GetColorForState(EButtonState State) const
{
	switch (State) {
	case EButtonState::Hovered:
		return m_HoveredColor;
	case EButtonState::Pressed:
		return m_PressedColor;
	case EButtonState::Normal:
	case EButtonState::Disabled:
	default:
		return m_NormalColor;
	}
}
