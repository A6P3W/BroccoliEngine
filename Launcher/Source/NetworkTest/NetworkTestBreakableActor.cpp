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
  BodySprite->SetRenderSettings(9, RenderSpace::World);
  BodySprite->SubmitBox(40.0f, 40.0f, FColor{255, 150, 40}, true);
  BodySprite->SetRelativeLocation({-20.0f, -20.0f});
  // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  BodySprite->RegisterComponent();

  // 衝突判定用の矩形コリジョンコンポーネントを作成
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  Collision->SetSize(40.0f, 40.0f);
  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を構築）する関数
  Collision->AttachToComponent(GetRootComponent());
  Collision->SetCollisionType(ECollisionType::Overlap);
  Collision->RegisterComponent();
}

void ANetworkTestBreakableActor::BeginOverlap(AActor* OtherActor) {
  // 基底クラスの BeginOverlap を実行
  AActor::BeginOverlap(OtherActor);

  // サーバー権限を持たない（クライアント）場合、またはすでに破棄処理中の場合は何もしない
  // （状態の変更や破棄などの重要処理はサーバー側でのみ判断します）
  if (!bHasAuthority || IsPendingDestroy()) {
    return;
  }

  // 衝突相手がプレイヤー（ANetworkTestPawn）の場合、アクターを破棄する
  if (dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    M_LOG("NetworkTestBreakableActor destroyed by player overlap: actor={}", NetworkId);
    // アクターの破棄処理。ネットワーク同期されているため、クライアント側のアクターも同期して破棄されます。
    Destroy();
  }
}
