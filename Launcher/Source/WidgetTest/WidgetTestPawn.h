#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;

// UI表示テスト時の動作検証用 Pawn クラス（APawn を継承）
class AWidgetTestPawn : public APawn {
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(AWidgetTestPawn)
 public:
  AWidgetTestPawn();
  void BeginPlay() override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUpdate(float DeltaTime) override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void OnInteractPressed();
  void OnMove(const FInputActionValue& Value);
  // 移動制御用のコンポーネント
  MMovementComponent* Movement = nullptr;
};
