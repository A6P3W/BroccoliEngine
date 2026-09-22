#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"

class MCamera2DComponent;
class MEnhancedInputComponent;
struct FInputActionValue;
class APlayerController;

class BROCCOLI_ENGINE_API APawn : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APawn);
  APawn();
  virtual void OnPossessedBy(APlayerController* NewController);
  virtual void OnUnPossessed();
  void OnUpdate(float DeltaTime) override;
  virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);

  APlayerController* GetController() const { return Controller; }

 private:
  void OnInteractPressed();
  virtual void OnMove(const FInputActionValue& Value);

 protected:
  MCamera2DComponent* Camera = nullptr;
  FVector2D ControlInputVector = {0, 0};
  APlayerController* Controller = nullptr;
};
