#pragma once

#include "ActorComponent.h"

class MDoorAutomationTestComponent final : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MDoorAutomationTestComponent)

  void SetActive(bool bInActive);
  bool IsActive() const;

 private:
  bool bActive = false;
};
