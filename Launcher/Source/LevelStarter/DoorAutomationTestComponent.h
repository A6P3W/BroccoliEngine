#pragma once

#include <nlohmann/json.hpp>

#include "ActorComponent.h"
#include "AutomationAnnotations.h"

class MDoorAutomationTestComponent final : public MActorComponent {
 private:
  static nlohmann::json ActiveToJson(bool Active);

 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MDoorAutomationTestComponent)

  CONTROL_METHOD(
          .Name = "set_active",
          .Description = "Sets the DoorActor automation test component active state."
  )
  CONTROL_PARAMETER(.Index = 0, .Name = "active", .Description = "New active state.")
  void SetActive(bool bInActive);
  CONTROL_METHOD(
          .Name = "is_active",
          .Description = "Returns the DoorActor automation test component active state.",
          .ResultAdapter = ^^MDoorAutomationTestComponent::ActiveToJson
  )
  bool IsActive() const;

 private:
  bool bActive = false;
};
