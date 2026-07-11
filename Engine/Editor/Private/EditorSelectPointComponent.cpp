#include "EditorSelectPointComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "SpriteComponent.h"
EditorSelectPointComponent::EditorSelectPointComponent() {}

void EditorSelectPointComponent::Draw() { SelectPointSprite->SetWorldScale(FScale(1.0f)); }

void EditorSelectPointComponent::Selected(bool bSelected) {
  int color = bSelected ? GetColor(255, 255, 0) : GetColor(255, 0, 0);
  SelectPointSprite->SubmitCircle(10, color, 1);
}

void EditorSelectPointComponent::OnRegister() {
  SelectPointSprite = NewObject<MSpriteComponent>(GetOwner());
  if (SelectPointSprite == nullptr) {
    return;
  }
  SelectPointSprite->SetRenderSettings(100, RenderSpace::World);
  SelectPointSprite->SetParentComponent(this);
  SelectPointSprite->SubmitCircle(8.0f, GetColor(255, 0, 0), true);
  SelectPointSprite->RegisterComponent();
}
