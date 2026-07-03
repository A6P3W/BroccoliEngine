#pragma once

#include "WidgetBase.h"

class MUIButtonComponent;

class ALevelStarterWidget : public AWidgetBase {
 public:
  DEFINE_ACTOR_CLASS(ALevelStarterWidget)

  ALevelStarterWidget();

 protected:
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
  MUIButtonComponent* OpenButton = nullptr;
  bool bWasMouseLeftDown = false;
};