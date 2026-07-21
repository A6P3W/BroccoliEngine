#pragma once
#include "Pawn.h"
#include "UMath.h"

class MCanvasComponent;
class MEnhancedInputComponent;
struct FInputActionValue;

// キャンバスお絵描き機能のテスト用アクター
class ACanvasTestActor : public APawn {
  DEFINE_ACTOR_CLASS(ACanvasTestActor)
 public:
  ACanvasTestActor();
  void BeginPlay() override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUpdate(float DeltaTime) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void OnMove(const FInputActionValue& Value);
  MCanvasComponent* Canvas = nullptr;
  FVector2D LastPaintPos = FVector2D::ZeroVector();
  bool bIsPaintingLastFrame = false;
};
