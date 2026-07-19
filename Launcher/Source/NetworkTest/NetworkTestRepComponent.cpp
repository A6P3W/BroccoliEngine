#include "NetworkTestRepComponent.h"

#include "Actor.h"
#include "Log.h"

namespace {
// RPC の一意な識別IDを定義
enum : FNetworkRPCId { RPC_ComponentServerTest = 101, RPC_ComponentMulticastTest = 102 };
}  // namespace

MNetworkTestRepComponent::MNetworkTestRepComponent() {
  // コンポーネント自体のネットワーク同期（レプリケーション）を有効化し、
  // サーバーとクライアント間でこのコンポーネントの状態や RPC を同期します。
  bReplicates = true;

  // ネットワーク同期時にこのコンポーネントを識別するためのユニークな識別名を設定するエンジンの関数。
  SetNetComponentName("NetworkTestRepComponent");

  // 指定したメンバ変数（ReplicatedCounter）をネットワーク同期プロパティとして登録し、
  // 値が変化した際に通知を受け取るコールバック関数（OnRepReplicatedCounter）をバインドするエンジンの関数。
  RegisterReplicatedProperty(
      &ReplicatedCounter, this, &MNetworkTestRepComponent::OnRepReplicatedCounter
  );

  // RPC（Remote Procedure Call：遠隔手続き呼び出し）関数を登録するエンジンの関数。
  // ここではサーバー側で実行する RPC（ENetRPCType::Server）を登録しています。
  RegisterRPC(
      RPC_ComponentServerTest,
      ENetRPCType::Server,
      this,
      &MNetworkTestRepComponent::Server_ComponentTest
  );

  // 全クライアントに対してサーバーから一斉送信（ブロードキャスト）する RPC 関数を登録。
  RegisterRPC(
      RPC_ComponentMulticastTest,
      ENetRPCType::Multicast,
      this,
      &MNetworkTestRepComponent::Multicast_ComponentTest
  );
}

void MNetworkTestRepComponent::RequestTest(int PlayerId) {
  // コンソールおよびログファイルにデバッグ用メッセージを出力するエンジンログマクロ。
  // GetOwner(): このコンポーネントを所有するアクターを取得する関数。
  // NetworkId: アクターが一意に持つネットワーク識別子。
  // ComponentNetworkId: コンポーネントが一意に持つネットワーク識別子。
  M_LOG(
      "Component RPC test input: actor={} component={} player={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId
  );

  // クライアント側からサーバー側へ登録された RPC (RPC_ComponentServerTest) の呼び出しを要求するエンジンの関数。
  // 信頼性設定には、パケットの確実な到達を保証する ENetPacketReliability::Reliable を指定。
  InvokeRPC(
      RPC_ComponentServerTest, ENetRPCType::Server, ENetPacketReliability::Reliable, PlayerId
  );
}

void MNetworkTestRepComponent::OnUpdate(float DeltaTime) {
  if (OnRepFlashTimer > 0.0f) {
    OnRepFlashTimer -= DeltaTime;
    if (OnRepFlashTimer < 0.0f) {
      OnRepFlashTimer = 0.0f;
    }
  }

  if (RPCFlashTimer > 0.0f) {
    RPCFlashTimer -= DeltaTime;
    if (RPCFlashTimer < 0.0f) {
      RPCFlashTimer = 0.0f;
    }
  }

  if (RPCFlashTimer <= 0.0f && PendingOnRepFlashTimer > 0.0f) {
    OnRepFlashTimer = PendingOnRepFlashTimer;
    PendingOnRepFlashTimer = 0.0f;
  }
}

void MNetworkTestRepComponent::Server_ComponentTest(int PlayerId) {
  // サーバー側でカウンタ値を増加
  ++ReplicatedCounter;

  // ネットワーク同期プロパティの値が変更され、クライアントへ再同期する必要があることをエンジンに示すための関数（サーバー側で呼び出します）。
  MarkReplicatedStateDirty();

  // GetOwner(): このコンポーネントを所有するアクターを取得する関数。
  // NetworkId: アクターが一意に持つネットワーク識別子。
  // ComponentNetworkId: コンポーネントが一意に持つネットワーク識別子。
  M_LOG(
      "Server_ComponentTest received: actor={} component={} player={} counter={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId,
      ReplicatedCounter
  );

  // サーバー側から全クライアントに向けてマルチキャスト RPC の実行を要求するエンジンの関数。
  InvokeRPC(
      RPC_ComponentMulticastTest,
      ENetRPCType::Multicast,
      ENetPacketReliability::Reliable,
      PlayerId,
      ReplicatedCounter
  );
}

void MNetworkTestRepComponent::Multicast_ComponentTest(int PlayerId, int CounterValue) {
  // GetOwner(): このコンポーネントを所有するアクターを取得する関数。
  // NetworkId: アクターが一意に持つネットワーク識別子。
  // ComponentNetworkId: コンポーネントが一意に持つネットワーク識別子。
  M_LOG(
      "Multicast_ComponentTest received: actor={} component={} player={} counter={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId,
      CounterValue
  );
  RPCFlashTimer = 0.8f;
}

void MNetworkTestRepComponent::OnRepReplicatedCounter(int OldValue) {
  // ネットワークレプリケーション（同期）によって ReplicatedCounter の値が更新された際に実行される処理
  // GetOwner(): このコンポーネントを所有するアクターを取得する関数。
  // NetworkId: アクターが一意に持つネットワーク識別子。
  // ComponentNetworkId: コンポーネントが一意に持つネットワーク識別子。
  M_LOG(
      "OnRep component counter: actor={} component={} old={} new={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      OldValue,
      ReplicatedCounter
  );
  if (RPCFlashTimer > 0.0f) {
    PendingOnRepFlashTimer = 0.75f;
  } else {
    OnRepFlashTimer = 0.75f;
  }
}
