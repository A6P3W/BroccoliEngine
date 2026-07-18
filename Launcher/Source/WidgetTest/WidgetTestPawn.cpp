#include "WidgetTestPawn.h"

#include <CircleCollisionComponent.h>
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include <MovementComponent.h>

#include "CameraComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "World.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(AWidgetTestPawn)

AWidgetTestPawn::AWidgetTestPawn() {
  // 衝突判定用の円形コリジョンコンポーネントを作成
  auto* col = NewObject<MCircleCollisionComponent>(this);
  col->SetRadius(32.0f);
  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を構築）する関数
  col->AttachToComponent(GetRootComponent());
  // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  col->RegisterComponent();

  // 指定したアクター（this）を所有者として、移動制御用の MMovementComponent を動的に作成するエンジンの関数
  Movement = NewObject<MMovementComponent>(this);
  // コンポーネントをエンジンシステムに登録し、アップデートなどの処理を有効化する関数
  Movement->RegisterComponent();
}
void AWidgetTestPawn::BeginPlay() {}
void AWidgetTestPawn::OnPossessedBy(APlayerController* NewController) {
  // 基底クラス APawn の OnPossessedBy（憑依）処理を実行
  APawn::OnPossessedBy(NewController);
  // アクターに標準搭載されているカメラコンポーネント（Camera）の視野パラメータを設定
  // （ここでは平行投影/フラット表示を行うような特殊設定としている）
  Camera->SetFOV(1);
}
void AWidgetTestPawn::OnUpdate(float DeltaTime) {
  // アクターの現在のワールド座標（2次元ベクトル）を取得
  const FVector2D PawnPosition = GetActorLocation();
  // 画面の固定位置に、デバッグ用の座標情報テキストを一定時間（0.1秒）表示するマクロ
  DRAW_SCREEN_LOG(
      "WidgetTestPawnPosition",
      0.1f,
      "WidgetTest Pawn Position: X={:.1f}, Y={:.1f}",
      PawnPosition.X,
      PawnPosition.Y
  );
  // アクターの頭上（ワールド座標）に、アクター名テキストを指定時間表示するマクロ
  DRAW_WORLD_LOG("WidgetTestPawnName", 0.1f, this, "WidgetTest Pawn");
  // アクターの頭上に、ワールド座標テキストを指定時間表示するマクロ
  DRAW_WORLD_LOG(
      "WidgetTestPawnWorldPosition",
      0.1f,
      this,
      "X={:.1f}, Y={:.1f}",
      PawnPosition.X,
      PawnPosition.Y
  );
}

void AWidgetTestPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  // インタラクトキーのアクション（開始時 ETriggerEvent::Started）にコールバックをバインド
  PlayerInputComponent->BindAction(
      InputAction::Interact, ETriggerEvent::Started, this, &AWidgetTestPawn::OnInteractPressed
  );
  // 移動キーのアクション（実行中 ETriggerEvent::Triggered）にコールバックをバインド
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &AWidgetTestPawn::OnMove
  );
}

void AWidgetTestPawn::OnInteractPressed() { M_LOG("F"); }

void AWidgetTestPawn::OnMove(const FInputActionValue& Value) {
  // 移動制御コンポーネントへ、ワールド座標系の力を加算
  Movement->AddWorldForce(Value.Axis2D * -2);
}
