#pragma once

#include "SceneComponent.h"

class MAttachmentRuleTestComponent : public MSceneComponent {
 public:
  MAttachmentRuleTestComponent();

 protected:
  void OnUpdate(float DeltaTime) override;
};