#include "NetworkTestPawn.h"

#include <DxLib.h>
#include <NetMovementComponent.h>
#include <RectangleCollisionComponent.h>

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "NetworkTestGameMode.h"
#include "NetworkTestRepComponent.h"
#include "NetworkTestUI.h"
#include "SpriteComponent.h"

// このアクタークラスを アクター登録システム (ActorRegistry) に一般アクターとして自動登録するマクロ。
REGISTER_ACTOR(ANetworkTestPawn)

ANetworkTestPawn::ANetworkTestPawn() {
  // アクターのネットワーク同期（レプリケーション）を有効化。
  // これによりサーバーとクライアント間でアクターの生成・破棄やトランスフォームの同期が行われます。
  bReplicates = true;

  // アクターに "ForceFieldAffected" タグを追加。
  // エンジン内の力場（ForceField）システムなどで、このアクターが力の影響を受けるべきかを判別するために使用します。
  AddTag("ForceFieldAffected");

  // デバッグ用のネットワーク接続 UI インスタンスを作成
  NetworkTestUI = std::make_unique<FNetworkTestUI>(*this);

  // 指定したアクター（this）を所有者として、描画用の MSpriteComponent を動的に作成するエンジンの関数。
  BodySprite = NewObject<MSpriteComponent>(this);

  // スプライトの描画優先順位（ソートキー: 10）と、描画を行う座標空間（RenderSpace::World）を設定する関数。
  BodySprite->SetRenderSettings(10, RenderSpace::World);

  // 指定されたサイズ（幅・高さ: 48.0f）、色（FColor）、および塗りつぶし設定で矩形（ボックス）の描画データを登録する関数。
  BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);

  // 親コンポーネント（ここではルート）に対する相対的な位置座標（FVector2D）を設定する関数。
  BodySprite->SetRelativeLocation({-24.0f, -24.0f});

  // コンポーネントをエンジンシステムに登録し、アップデートや描画などのライフサイクル処理を有効化する関数。
  BodySprite->RegisterComponent();

  // 物理衝突判定を担当する MRectangleCollisionComponent を動的に作成。
  auto* CollisionComponent = NewObject<MRectangleCollisionComponent>(this);

  // 衝突判定用矩形（ボックス）のサイズ（幅・高さ）を設定する関数。
  CollisionComponent->SetSize(48.0f, 48.0f);

  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を設定）する関数。
  CollisionComponent->AttachToComponent(GetRootComponent());

  // コリジョンを動的（移動可能）オブジェクトとして設定する関数（false にすると静的/固定オブジェクトになる）。
  CollisionComponent->SetStatic(false);

  // コリジョンコンポーネントを登録し、衝突判定処理を有効化する関数。
  CollisionComponent->RegisterComponent();

  // ネットワーク同期に対応した移動制御コンポーネント MNetMovementComponent を動的に作成。
  Movement = NewObject<MNetMovementComponent>(this);

  // 移動コンポーネントを登録。
  Movement->RegisterComponent();

  // ネットワーク同期および RPC テスト用のカスタムコンポーネントを動的に作成。
  ReplicationTest = NewObject<MNetworkTestRepComponent>(this);

  // カスタムコンポーネントを登録。
  ReplicationTest->RegisterComponent();
}

ANetworkTestPawn::~ANetworkTestPawn() = default;

void ANetworkTestPawn::BeginPlay() {
  // 基底クラス APawn の BeginPlay 処理を実行。
  APawn::BeginPlay();

  // この Pawn がローカルプレイヤーによって操作されている（ローカルクライアントである）場合のみ、デバッグ UI を初期化。
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->InitializeStatus();
  }
}

void ANetworkTestPawn::OnPossessedBy(APlayerController* NewController) {
  // 基底クラス APawn の OnPossessedBy（プレイヤー憑依時イベント）処理を実行。
  APawn::OnPossessedBy(NewController);
}

void ANetworkTestPawn::OnUpdate(float DeltaTime) {
  if (FlashTimer > 0.0f) {
    FlashTimer -= DeltaTime;
    if (FlashTimer < 0.0f) {
      FlashTimer = 0.0f;
    }
  }

  if (BodySprite) {
    // 指定されたサイズ（幅・高さ: 48.0f）、色（FColor）、および塗りつぶし設定で矩形（ボックス）の描画データを毎フレーム登録する関数。
    BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
  }
}

void ANetworkTestPawn::Draw() {
  // 基底クラス APawn の Draw 処理を実行。
  APawn::Draw();

  // ローカルプレイヤーが操作している場合のみ、接続状況などのデバッグ UI を描画。
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->Draw();
  }
}

void ANetworkTestPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  if (!PlayerInputComponent) {
    return;
  }

  // 移動アクション（InputAction::Move）がトリガーされた際（ETriggerEvent::Triggered）に呼び出す関数（OnMove）をバインド。
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &ANetworkTestPawn::OnMove
  );
  // インタラクトアクション（InputAction::Interact）が開始された際（ETriggerEvent::Started）に呼び出す関数（OnInteract）をバインド。
  PlayerInputComponent->BindAction(
      InputAction::Interact, ETriggerEvent::Started, this, &ANetworkTestPawn::OnInteract
  );
}

void ANetworkTestPawn::OnMove(const FInputActionValue& Value) {
  // ローカル操作されているアクターでない場合は入力を処理しない。
  if (!bIsLocallyControlled || !Movement) {
    return;
  }

  // 同期移動コンポーネントに対して、ワールド座標系で入力に応じた推進力を加える関数。
  Movement->AddWorldForce(Value.Axis2D * -1.0f);
}

void ANetworkTestPawn::OnInteract(const FInputActionValue& Value) {
  (void)Value;

  if (!bIsLocallyControlled) {
    return;
  }
}

FColor ANetworkTestPawn::GetDisplayColor() const {
  if (ReplicationTest && ReplicationTest->IsRPCFlashActive()) {
    return FColor{255, 120, 255};
  }

  if (ReplicationTest && ReplicationTest->IsOnRepFlashActive()) {
    return FColor{80, 170, 255};
  }

  if (FlashTimer > 0.0f) {
    return FColor{255, 255, 80};
  }

  // 自アクターがローカル操作対象（bIsLocallyControlled）か、あるいはサーバー側のホストプレイヤー（bHasAuthority 且つ接続IDが0）なら緑、他クライアントなら赤。
  // bHasAuthority: 現在の実行環境がサーバー（オーソリティを持つ側）かどうかを判定。
  // OwnerConnectionId: このアクターを所有しているクライアントのネットワーク接続ID（サーバーホスト自身は 0）。
  return (bIsLocallyControlled || (bHasAuthority && OwnerConnectionId == 0)) ? FColor{0, 220, 80}
                                                                             : FColor{220, 60, 60};
}

void ANetworkTestPawn::BeginOverlap(AActor* OtherActor) {
  // 基底クラス AActor の BeginOverlap を実行。
  AActor::BeginOverlap(OtherActor);

  if (auto OtherPawn = dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    // コンソールおよびログファイルにデバッグ用メッセージを出力するエンジンログマクロ
    // NetworkId: アクターが一意に持つネットワーク識別子
    M_LOG(
        "Player overlap detected! My NetworkId: {}, Other NetworkId: {}",
        NetworkId,
        OtherPawn->NetworkId
    );
  }
}

void ANetworkTestPawn::SetStatusMessage(const std::string& Message) {
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->SetStatusMessage(Message);
  }
}
