#include "CollisionTestActors.h"

#include "CircleCollisionComponent.h"
#include "CollisionComponent.h"
#include "LineCollisionComponent.h"
#include "Log.h"
#include "RectangleCollisionComponent.h"
#include "SpriteComponent.h"

// 各アクタークラスを ActorRegistry にアクターとして自動登録するマクロ
REGISTER_ACTOR(ACollisionTestRectangleActor)
REGISTER_ACTOR(ACollisionTestBlockRectangleActor)
REGISTER_ACTOR(ACollisionTestCircleActor)
REGISTER_ACTOR(ACollisionTestLineActor)

void ACollisionTestActorBase::ConfigureCollision(
    MCollisionComponent* Collision, const char* ShapeName
) {
  ShapeLabel = ShapeName;
  // コリジョンの反応タイプを設定（ECollisionType::Overlap
  // は重なりを検知し、物理的押し戻しは行わない）
  Collision->SetCollisionType(ECollisionType::Overlap);
  // コリジョンを動的（移動・回転可能）オブジェクトとして設定
  Collision->SetStatic(false);
  // ルートコンポーネントにアタッチ
  Collision->AttachToComponent(GetRootComponent());
  // コリジョンコンポーネントを登録し、衝突判定を有効化
  Collision->RegisterComponent();
}

void ACollisionTestActorBase::OnUpdate(float DeltaTime) {
  // 基底クラス AActor の OnUpdate 処理を実行
  AActor::OnUpdate(DeltaTime);
  ElapsedSeconds += DeltaTime;
  // アクターに回転（FRotator）を加える関数
  // （ピッチ、ヨー、ロールからなる回転値を指定。ここではZ軸方向の回転）
  AddActorRotation(FRotator(25.0f * DeltaTime));
  // アクターの位置に、デバッグ用のログ文字列を一定時間（0.1秒）表示するマクロ
  DRAW_WORLD_LOG("CollisionShape", 0.1f, this, "{}: overlap / rotating", ShapeLabel);
  // 画面の固定位置に、デバッグ用の説明テキストを表示するマクロ
  DRAW_SCREEN_LOG(
      "CollisionHelp", 0.1f, "CollisionTest: collision shapes are debug-drawn; Escape LevelStarter"
  );
}

void ACollisionTestActorBase::BeginOverlap(AActor* OtherActor) {
  // 衝突した相手のアクターのクラス名（GetActorClassName()）を含むログを出力
  M_LOG("{} BeginOverlap with {}", ShapeLabel, OtherActor->GetActorClassName());
  // 衝突検知の発生をアクターのワールド座標位置にデバッグ描画（1.0秒間表示）
  DRAW_WORLD_LOG("CollisionBegin", 1.0f, this, "BeginOverlap: {}", OtherActor->GetActorClassName());
}

void ACollisionTestActorBase::EndOverlap(AActor* OtherActor) {
  // 衝突状態から離脱した相手のアクター名を含むログを出力
  M_LOG("{} EndOverlap with {}", ShapeLabel, OtherActor->GetActorClassName());
  // 離脱の発生をワールド座標位置にデバッグ描画（1.0秒間表示）
  DRAW_WORLD_LOG("CollisionEnd", 1.0f, this, "EndOverlap: {}", OtherActor->GetActorClassName());
}

ACollisionTestRectangleActor::ACollisionTestRectangleActor() {
  // 指定したアクター（this）を所有者として、矩形コリジョンコンポーネント（MRectangleCollisionComponent）を動的に作成するエンジンの関数
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  // 矩形コリジョンのサイズ（幅: 180.0f, 高さ: 90.0f）を設定する関数
  Collision->SetSize(180.0f, 90.0f);
  ConfigureCollision(Collision, "Rectangle");
}

ACollisionTestBlockRectangleActor::ACollisionTestBlockRectangleActor() {
  // 指定したアクター（this）を所有者として、矩形コリジョンコンポーネントを動的に作成
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  // 矩形コリジョンのサイズを設定
  Collision->SetSize(130.0f, 70.0f);
  ConfigureCollision(Collision, "Rectangle (Block)");
  // コリジョンの反応タイプを ECollisionType::Block（他のオブジェクトと物理的に衝突して押し戻す）に設定
  Collision->SetCollisionType(ECollisionType::Block);
}
ACollisionTestCircleActor::ACollisionTestCircleActor() {
  // 指定したアクター（this）を所有者として、円形コリジョンコンポーネント（MCircleCollisionComponent）を動的に作成するエンジンの関数
  auto* Collision = NewObject<MCircleCollisionComponent>(this);
  // コリジョンの半径を設定する関数
  Collision->SetRadius(55.0f);
  ConfigureCollision(Collision, "Circle");
}

ACollisionTestLineActor::ACollisionTestLineActor() {
  // 指定したアクター（this）を所有者として、線分コリジョンコンポーネント（MLineCollisionComponent）を動的に作成するエンジンの関数
  auto* Collision = NewObject<MLineCollisionComponent>(this);
  // 線分コリジョンの始点と終点（ローカル座標系）を設定する関数
  Collision->SetLine({-90.0f, 0.0f}, {90.0f, 0.0f});
  ConfigureCollision(Collision, "Line");
}
