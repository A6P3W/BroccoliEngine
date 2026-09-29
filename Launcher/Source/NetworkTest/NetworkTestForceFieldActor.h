#pragma once

#include "ForceFieldActor.h"

class MSprite2DComponent;

// 他のアクターに対して物理的な力を及ぼす力場アクター AForceFieldActor を継承
class ANetworkTestForceFieldActor : public AForceFieldActor {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ANetworkTestForceFieldActor)
  ANetworkTestForceFieldActor();

 private:
  MSprite2DComponent* FieldMarker = nullptr;
};
