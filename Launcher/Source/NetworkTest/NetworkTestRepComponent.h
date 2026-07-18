#pragma once

#include "ActorComponent.h"

// アクターにアタッチされ、ネットワーク同期や RPC（遠隔手続き呼び出し）の検証を行うカスタムコンポーネント
// アクターにアタッチして機能を拡張するコンポーネントのエンジンの基底クラス MActorComponent を継承します
class MNetworkTestRepComponent : public MActorComponent {
 public:
  MNetworkTestRepComponent();

  void RequestTest(int PlayerId);
  int GetReplicatedCounter() const { return ReplicatedCounter; }
  bool IsOnRepFlashActive() const { return OnRepFlashTimer > 0.0f; }
  bool IsRPCFlashActive() const { return RPCFlashTimer > 0.0f; }

 protected:
  // 毎フレーム呼び出されるコンポーネント用の更新ライフサイクル関数
  // DeltaTime は前フレームからの経過時間（秒）
  void OnUpdate(float DeltaTime) override;

 private:
  // サーバー側で実行される RPC 関数
  void Server_ComponentTest(int PlayerId);
  // サーバーから全クライアントに対してブロードキャスト実行される RPC 関数
  void Multicast_ComponentTest(int PlayerId, int CounterValue);
  // 同期プロパティ ReplicatedCounter の値がネットワーク経由で更新された際に呼び出されるコールバック（OnRep）関数
  void OnRepReplicatedCounter(int OldValue);

  // ネットワーク同期される変数
  int ReplicatedCounter = 0;
  float OnRepFlashTimer = 0.0f;
  float RPCFlashTimer = 0.0f;
  float PendingOnRepFlashTimer = 0.0f;
};
