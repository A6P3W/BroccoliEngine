#include "EditorSelectPointComponent.h"

#include <DxLib.h>

#include "Actor.h"
#include "SpriteComponent.h"
EditorSelectPointComponent::EditorSelectPointComponent() {}

void EditorSelectPointComponent::Draw() { SelectPointSprite->SetWorldScale(FScale(1.0f)); }

void EditorSelectPointComponent::Selected(bool bSelected) {
  FColor color = bSelected ? FColor{255, 255, 0} : FColor{255, 0, 0};
  SelectPointSprite->SubmitCircle(10, color, 1);
}

void EditorSelectPointComponent::OnRegister() {
  SelectPointSprite = NewObject<MSpriteComponent>(GetOwner());
  if (SelectPointSprite == nullptr) {
    return;
  }
  SelectPointSprite->SetRenderSettings(100, RenderSpace::World);
  SelectPointSprite->AttachToComponent(this);
  SelectPointSprite->SubmitCircle(8.0f, FColor{255, 0, 0}, true);
  SelectPointSprite->RegisterComponent();
}
