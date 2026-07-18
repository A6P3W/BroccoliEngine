#include "SceneLifecycleActor.h"

#include "Log.h"
#include "SpriteComponent.h"
#include "World.h"

// 各アクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ASceneLifecycleActor)
REGISTER_ACTOR(ASceneSpawnedMarkerActor)

void ASceneLifecycleActor::BeginPlay() {
  AActor::BeginPlay();
  // タイマーマネージャーを利用して、ToggleSpawnedActor 関数を 3.0秒周期でループ(true)実行するようにタイマーを設定
  GetWorldTimerManager().SetTimer(
      LifecycleTimer, this, &ASceneLifecycleActor::ToggleSpawnedActor, 3.0f, true
  );
  ToggleSpawnedActor();
  M_LOG("SceneLifecycleActor: a marker is spawned/destroyed every three seconds.");
}

void ASceneLifecycleActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  DRAW_SCREEN_LOG(
      "SceneHelp", 0.1f, "SceneTest: attachment rules + Spawn/Destroy marker / Escape LevelStarter"
  );
  DRAW_WORLD_LOG("SceneLifecycle", 0.1f, this, "Spawn/Destroy demonstration");
}

void ASceneLifecycleActor::ToggleSpawnedActor() {
  // 生成済みのアクターが存在し、かつそれが破棄中ではない場合、そのアクターを破棄する
  if (SpawnedActor && !SpawnedActor->IsPendingDestroy()) {
    M_LOG("SceneTest: destroy spawned marker.");
    // アクターをワールドから完全に破棄
    SpawnedActor->Destroy();
    SpawnedActor = nullptr;
    return;
  }

  // アクターが存在しない場合、アクターを現在の位置から右に 140.0f オフセットした位置に生成する
  if (GetWorld()) {
    // テンプレート関数 SpawnActor を用いてワールドに ASceneSpawnedMarkerActor インスタンスを作成
    SpawnedActor = GetWorld()->SpawnActor<ASceneSpawnedMarkerActor>(
        GetActorLocation() + FVector2D{140.0f, 0.0f}
    );
    M_LOG("SceneTest: spawn marker actor.");
  }
}

ASceneSpawnedMarkerActor::ASceneSpawnedMarkerActor() {
  // 指定したアクター（this）を所有者として、描画用のスプライトコンポーネント（MSpriteComponent）を動的に作成するエンジンの関数
  auto* Sprite = NewObject<MSpriteComponent>(this);
  Sprite->SetRenderSettings(20, RenderSpace::World);
  Sprite->SubmitCircle(18.0f, FColor{255, 180, 60}, true);
  // このコンポーネントをアクターのルートコンポーネントにアタッチ（親子関係を構築）する関数
  Sprite->AttachToComponent(GetRootComponent());
  // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
  Sprite->RegisterComponent();
}

void ASceneSpawnedMarkerActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  // アクターに毎フレーム回転運動を加える
  AddActorRotation(FRotator(90.0f * DeltaTime));
  DRAW_WORLD_LOG("SceneSpawnedMarker", 0.1f, this, "Spawned actor");
}
