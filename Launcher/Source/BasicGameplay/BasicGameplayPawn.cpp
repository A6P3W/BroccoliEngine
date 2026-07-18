#include "BasicGameplayPawn.h"

#include "CircleCollisionComponent.h"
#include "EnhancedInputComponent.h"
#include "Log.h"
#include "MovementComponent.h"
#include "SpriteComponent.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ABasicGameplayPawn)

ABasicGameplayPawn::ABasicGameplayPawn() {
  // 指定したアクター（this）を所有者として、描画用の MSpriteComponent を動的に作成
  auto* Body = NewObject<MSpriteComponent>(this);
  // 描画順の優先度（10）と、描画空間をワールド座標（RenderSpace::World）に設定
  Body->SetRenderSettings(10, RenderSpace::World);
  // 半径 24.0f、水色（R:80, G:190, B:255）、塗りつぶしあり(true) の円形描画データを登録
  Body->SubmitCircle(24.0f, FColor{80, 190, 255}, true);
  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を設定）
  Body->AttachToComponent(GetRootComponent());
  // コンポーネントをエンジンシステムに登録し、レンダリングなどの処理を有効化
  Body->RegisterComponent();

  // 円形の物理衝突判定を担当する MCircleCollisionComponent を動的に作成
  auto* Collision = NewObject<MCircleCollisionComponent>(this);
  // 衝突判定用の半径を 24.0f に設定
  Collision->SetRadius(24.0f);
  // コリジョンを動的（移動可能）オブジェクトとして設定（false にすると静的/固定オブジェクトになる）
  Collision->SetStatic(false);
  // ルートコンポーネントにアタッチ
  Collision->AttachToComponent(GetRootComponent());
  // コリジョンコンポーネントを登録し、物理挙動を有効化
  Collision->RegisterComponent();

  // アクターの移動挙動を制御する MMovementComponent を動的に作成
  Movement = NewObject<MMovementComponent>(this);
  // 移動時の摩擦力（減衰率）を 0.90f に設定
  Movement->SetFriction(0.90f);
  // 移動コンポーネントを登録
  Movement->RegisterComponent();
}

void ABasicGameplayPawn::BeginPlay() {
  // 基底クラス APawn の BeginPlay 処理を実行
  APawn::BeginPlay();
  // コンソールおよびログファイルにデバッグ用メッセージを出力するエンジンマクロ
  M_LOG("BasicGameplayPawn: WASD moves the pawn; the pawn camera follows automatically.");
}

void ABasicGameplayPawn::OnUpdate(float DeltaTime) {
  // 基底クラス APawn の OnUpdate 処理を実行
  APawn::OnUpdate(DeltaTime);
  // アクターの現在のワールド座標（2次元ベクトル）を取得
  const FVector2D Position = GetActorLocation();
  // 画面の固定位置に、デバッグ用のテキストを指定時間（0.1秒）表示するマクロ
  DRAW_SCREEN_LOG("BasicGameplayHelp", 0.1f, "BasicGameplay: WASD move / Escape LevelStarter");
  // アクターの頭上（ワールド座標）に、座標情報テキストを指定時間（0.1秒）表示するマクロ
  DRAW_WORLD_LOG(
      "BasicGameplayPosition", 0.1f, this, "Pawn ({:.0f}, {:.0f})", Position.X, Position.Y
  );
}

void ABasicGameplayPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  if (PlayerInputComponent) {
    // 入力アクション（Move）がトリガーされた際（ETriggerEvent::Triggered）に、
    // 指定したメンバ関数（ABasicGameplayPawn::Move）を呼び出すようにバインド
    PlayerInputComponent->BindAction(
        InputAction::Move, ETriggerEvent::Triggered, this, &ABasicGameplayPawn::Move
    );
  }
}

void ABasicGameplayPawn::Move(const FInputActionValue& Value) {
  if (Movement) {
    // 移動コンポーネントに対して、入力値（2D移動ベクトル）に基づいた力を加える
    Movement->AddWorldForce(Value.Axis2D * -2.0f);
  }
}
