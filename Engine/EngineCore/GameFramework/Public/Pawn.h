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
  /// @brief PawnがControllerによって操作を開始した。
  /// @param NewController このPawnを操作するController
  virtual void OnPossessedBy(APlayerController* NewController);
  /// @brief ControllerがこのPawnの操作をやめた。
  virtual void OnUnPossessed();
  /// @brief 更新処理を記述する。
  /// @param DeltaTime
  void OnUpdate(float DeltaTime) override;
  /// @brief OnPosessedByの後に呼ばれる。comp->BindActionを行い、アクションバインド処理を記述。
  /// @param PlayerInputComponent
  virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);

  /// @brief このPawnを操作中のControllerを取得。
  /// @return
  APlayerController* GetController() const { return Controller; }

 private:
  void OnInteractPressed();
  virtual void OnMove(const FInputActionValue& Value);

 protected:
  MCamera2DComponent* Camera = nullptr;
  FVector2D ControlInputVector = {0, 0};
  APlayerController* Controller = nullptr;
};
