#pragma once

#include "ActorComponent.h"

class FAutomationComponentMethodRegistry;

class MDoorAutomationTestComponent final : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MDoorAutomationTestComponent)

  void SetActive(bool bInActive);
  bool IsActive() const;

  static void RegisterAutomationMethods(FAutomationComponentMethodRegistry& Registry);

 private:
  bool bActive = false;
};
