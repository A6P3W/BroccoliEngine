#include "NetworkTestForceFieldActor.h"

#include <DxLib.h>

#include "CircleCollisionComponent.h"
#include "ForceFieldComponent.h"
#include "SpriteComponent.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ANetworkTestForceFieldActor)

ANetworkTestForceFieldActor::ANetworkTestForceFieldActor() {
  // アクターが持つ力場制御コンポーネントを取得
  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (ForceField) {
    // 力場の種類を「一方向への力（指向性力場）」に設定
    ForceField->SetForceType(EForceFieldType::Directional);
    // 力の方向ベクトルを設定（X軸プラス方向）
    ForceField->SetDirection({1.0f, 0.0f});
    // 力の強さを設定
    ForceField->SetStrength(0.5f);
    // この力場の影響を受けるアクターが持つべきタグを設定
    ForceField->SetAffectedActorTags({"ForceFieldAffected"});
    // 力場を有効化
    ForceField->SetActive(true);
  }

  // 力場の範囲を規定するコリジョンコンポーネントを取得
  MCircleCollisionComponent* Range = dynamic_cast<MCircleCollisionComponent*>(GetRangeComponent());
  if (Range) {
    // 影響が及ぶ円の半径を設定
    Range->SetRadius(160.0f);
  }

  // 指定したアクター（this）を所有者として、描画用のスプライトコンポーネント（MSpriteComponent）を動的に作成するエンジンの関数
  FieldMarker = NewObject<MSpriteComponent>(this);
  if (FieldMarker) {
    FieldMarker->SetRenderSettings(8, RenderSpace::World);
    FieldMarker->SubmitCircle(18.0f, FColor{80, 190, 255}, true);
    // コンポーネントをエンジンシステムに登録し、初期化やアップデート、レンダリングなどのライフサイクル処理の対象にする関数
    FieldMarker->RegisterComponent();
  }
}
