#pragma once
#include <InputMapper.h>

#include <memory>

#include "Actor.h"
#include "BroccoliEngineAPI.h"
#include "Pawn.h"

enum class EInputMode { GameOnly, UIOnly, GameAndUI };
class MEnhancedInputComponent;
class BROCCOLI_ENGINE_API APlayerController : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APlayerController)
  APlayerController();
  ~APlayerController() override;
  /// @brief Pawnの操作を開始。
  /// @param NewPawn 操作対象のPawn
  virtual void Possess(APawn* NewPawn);

  void OnUpdate(float DeltaTime) override;
  void SetPlayerId(int id);
  int GetPlayerId() const;
  virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);
  virtual void SetupInputMappings();
  InputMapper* GetInputMapper();
  MEnhancedInputComponent* GetInputComponent();
  /// @brief 操作中のPawnを取得
  /// @return
  APawn* GetPawn() const;

  /// @brief 入力モードを変更
  /// @param NewInputMode このコントローラーがどの領域を操作するか
  void SetInputMode(EInputMode NewInputMode);
  EInputMode GetInputMode() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
