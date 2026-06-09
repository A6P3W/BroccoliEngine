#include "EditorSelectPointComponent.h"
#include "SpriteComponent.h"
#include <DxLib.h>
#include "Actor.h"
EditorSelectPointComponent::EditorSelectPointComponent()
{

}


void EditorSelectPointComponent::Draw()
{
	SelectPointSprite->SetWorldScale(1);
}

void EditorSelectPointComponent::Selected(bool bSelected)
{
	int color = bSelected ? GetColor(255, 255, 0) : GetColor(255, 0, 0);
	SelectPointSprite->SubmitCircle(10, color, 1);
}

void EditorSelectPointComponent::RegisterComponent()
{
	auto sprite = std::make_unique<MSpriteComponent>(100);
	sprite->SubmitCircle(10, GetColor(255, 0, 0), 1);
	SelectPointSprite = sprite.get();

	GetOwner()->AddComponent(std::move(sprite));
	SelectPointSprite->SetParentComponent(this);
}
