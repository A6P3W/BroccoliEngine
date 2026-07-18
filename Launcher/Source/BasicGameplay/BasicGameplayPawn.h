#pragma once

#include "Pawn.h"

class MEnhancedInputComponent;
class MMovementComponent;
struct FInputActionValue;

// プレイヤーやAIが憑依（Possess）して操作可能な、エンジンのポーン基底クラス APawn を継承
class ABasicGameplayPawn : public APawn {
 public:
  // クラスのメタデータ（型名 StaticClassName(), クラス名取得 GetActorClassName()
  // など）を定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ABasicGameplayPawn)

  ABasicGameplayPawn();

  // アクターがゲーム内に配置・生成され、プレイが開始されたときに一度だけ呼び出されるライフサイクル関数
  void BeginPlay() override;

  // 毎フレーム呼び出される更新用ライフサイクル関数。DeltaTime は前フレームからの経過時間（秒）
  void OnUpdate(float DeltaTime) override;

  // プレイヤーからの入力をこの Pawn のアクション（Moveなど）にバインドする設定用ライフサイクル関数
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void Move(const FInputActionValue& Value);
  // アクターに物理的な移動機能を提供する移動コンポーネントのポインタ
  MMovementComponent* Movement = nullptr;
};
