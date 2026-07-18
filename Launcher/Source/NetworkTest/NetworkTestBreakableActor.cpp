#include "NetworkTestBreakableActor.h"

#include <DxLib.h>

#include <memory>

#include "CollisionComponent.h"
#include "Log.h"
#include "NetworkTestPawn.h"
#include "RectangleCollisionComponent.h"
#include "SpriteComponent.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ANetworkTestBreakableActor)

ANetworkTestBreakableActor::ANetworkTestBreakableActor() {
  // ネットワーク同期（レプリケーション）を有効化
  // これによりサーバーとクライアント間でアクターの生成・破棄やプロパティの同期が行われます
  bReplicates = true;

  // 指定したアクター（this）を所有者として、描画用のスプライトコンポーネント（MSpriteComponent）を動的に作成するエンジンの関数
  BodySprite = NewObject<MSpriteComponent>(this);
  // 描画順の優先度（9）と、描画空間をワールド座標（RenderSpace::World）に設定する関数
  BodySprite->SetRenderSettings(9, RenderSpace::World);
  // 指定されたサイズ（幅・高さ: 40.0f）、色（FColor）、および塗りつぶし設定で矩形（ボックス）の描画データを登録する関数
  BodySprite->SubmitBox(40.0f, 40.0f, FColor{255, 150, 40}, true);
  // 親コンポーネント（ここではルート）に対する相対的な位置座標（FVector2D）を設定する関数
  BodySprite->SetRelativeLocation({-20.0f, -20.0f});
  // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  BodySprite->RegisterComponent();

  // 衝突判定用の矩形コリジョンコンポーネントを作成
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  // 衝突判定用矩形（ボックス）のサイズ（幅・高さ）を設定する関数
  Collision->SetSize(40.0f, 40.0f);
  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を構築）する関数
  Collision->AttachToComponent(GetRootComponent());
  // 衝突タイプを「Overlap」（重なり判定のみで押し出し物理は行わない）に設定する関数
  Collision->SetCollisionType(ECollisionType::Overlap);
  // コリジョンコンポーネントを登録し、衝突判定処理を有効化する関数
  Collision->RegisterComponent();
}

void ANetworkTestBreakableActor::BeginOverlap(AActor* OtherActor) {
  // 基底クラスの BeginOverlap を実行する関数
  AActor::BeginOverlap(OtherActor);

  // サーバー権限を持たない（クライアント）場合、またはすでに破棄処理中の場合は何もしない
  // （状態の変更や破棄などの重要処理はサーバー側でのみ判断します）
  // bHasAuthority: 現在の実行環境がサーバー（オーソリティを持つ側）かどうかを判定する変数
  // IsPendingDestroy: このアクターが既に破棄マーク（削除予定）されているかを判定する関数
  if (!bHasAuthority || IsPendingDestroy()) {
    return;
  }

  // 衝突相手がプレイヤー（ANetworkTestPawn）の場合、アクターを破棄する
  if (dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    // コンソールおよびログファイルにデバッグ用メッセージを出力するエンジンログマクロ
    // NetworkId: アクターが一意に持つネットワーク識別子
    M_LOG("NetworkTestBreakableActor destroyed by player overlap: actor={}", NetworkId);
    // アクターの破棄処理。ネットワーク同期されているため、クライアント側のアクターも同期して破棄されます。
    Destroy();
  }
}
