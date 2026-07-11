#include "AttachmentRuleTestComponent.h"

MAttachmentRuleTestComponent::MAttachmentRuleTestComponent() = default;

void MAttachmentRuleTestComponent::OnUpdate(float DeltaTime) {
  AddWorldRotation(FRotator(30.0f * DeltaTime));
}