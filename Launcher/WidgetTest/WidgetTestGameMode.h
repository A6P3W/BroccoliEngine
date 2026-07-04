#pragma once
#include "GameModeBase.h"
class AWidgetTestGameMode : public AGameModeBase {
 public:
  DEFINE_ACTOR_CLASS(AWidgetTestGameMode)
  AWidgetTestGameMode();
  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
};
