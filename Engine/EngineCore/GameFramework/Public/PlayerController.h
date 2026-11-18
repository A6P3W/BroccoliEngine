#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"

class InputMapper;
class APawn;

enum class EInputMode { GameOnly, UIOnly, GameAndUI };
class MEnhancedInputComponent;
class BROCCOLI_ENGINE_API APlayerController : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APlayerController)
  APlayerController();
  ~APlayerController() override;
  virtual void Possess(APawn* NewPawn);

  void OnUpdate(float DeltaTime) override;
  void SetPlayerId(int id);
  int GetPlayerId() const;
  virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);
  virtual void SetupInputMappings();
  InputMapper* GetInputMapper();
  MEnhancedInputComponent* GetInputComponent();
  APawn* GetPawn() const;

  void SetInputMode(EInputMode NewInputMode);
  EInputMode GetInputMode() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
