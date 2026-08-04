# BroccoliEngine ネットワーク同期・RPC・トランスポート 開発ガイド

## 1. 概要

BroccoliEngineのネットワーク層は、ゲーム状態同期を行うReplication層と、パケット配送を担当するTransport層を分離しています。

主な機能は次のとおりです。

- ActorおよびComponentの状態同期
- 変更されたプロパティだけを送る差分レプリケーション
- `OnRep` コールバック
- Actor RPCとComponent RPC
- Server / Client / Multicast RPC
- Reliable / Unreliable配送指定
- ENetとEOS P2Pの切り替え

## 2. ネットワークモード

Worldは次のモードを持ちます。

```cpp
enum class ENetMode {
  Standalone,
  ListenServer,
  Client
};
```

| モード | 説明 |
|---|---|
| `Standalone` | ネットワーク同期なしのローカル実行 |
| `ListenServer` | ホストがサーバーとローカルプレイヤーを兼任 |
| `Client` | リモートサーバーへ接続するクライアント |

サーバー権限が必要な状態変更は、`ListenServer` 側で実行してください。

## 3. Componentのレプリケーション有効化

Componentで同期を行う場合、コンストラクタでレプリケーションを有効化します。

```cpp
MHealthComponent::MHealthComponent() {
  bReplicates = true;
  SetNetComponentName("HealthComponent");
}
```

`SetNetComponentName()` には、同一Actor内で一意かつ安定した名前を指定します。この名前はサーバーとクライアントで同じComponentを対応付けるために使用されます。

動的値やInstanceNameをComponent名へ含めないでください。

## 4. Replicated Property

### 4.1 コールバックなし

```cpp
RegisterReplicatedProperty(&CurrentShield);
```

### 4.2 OnRepコールバックあり

```cpp
RegisterReplicatedProperty(
    &CurrentHealth,
    this,
    &MHealthComponent::OnRepCurrentHealth
);
```

コールバックのシグネチャは、同期対象型の旧値を値渡しで受け取ります。

```cpp
void MHealthComponent::OnRepCurrentHealth(float OldValue) {
  const float NewValue = CurrentHealth;
  UpdateHealthBar(OldValue, NewValue);
}
```

### 4.3 変更検出

登録時に現在値がキャッシュされます。送信判定時に `operator!=` でキャッシュ値と比較し、変更されたプロパティだけをシリアライズします。

送信後はキャッシュが更新されます。

### 4.4 対応型

標準実装では次の条件を満たす型を登録できます。

- `std::string`
- trivially copyableな型
- `operator!=` を利用できる型

独自型を使う場合、メモリレイアウト、エンディアン、パディング、バージョン互換性を考慮してください。可搬性が必要なら明示的なシリアライズを追加します。

### 4.5 サーバー側の更新

```cpp
void MHealthComponent::ApplyDamage(float Damage) {
  if (!GetOwner() || !GetOwner()->GetWorld()->IsListenServer()) {
    return;
  }

  CurrentHealth = std::max(0.0f, CurrentHealth - Damage);
  MarkReplicatedStateDirty();
}
```

値を変更した後、同期対象が更新されたことをReplication Systemへ通知します。

## 5. OnRepの用途

`OnRep` はクライアントが新しい値を受信し、旧値と異なる場合に呼ばれます。

適した処理：

- UI更新
- エフェクト再生
- 表示状態の変更
- 補間開始
- ローカルキャッシュ再構築

避ける処理：

- サーバー権限が必要なゲーム状態変更
- 同じReplicated Propertyの無条件な再変更
- 永続データ保存
- RPCの無限連鎖

## 6. Component RPC

ComponentはActorを経由してReplication SystemへRPCを送信します。パケットにはActorのNetwork IDとComponent Network IDが含まれ、受信側で対象Componentへ配送されます。

### 6.1 RPC ID

RPC IDはクラス内で一意に定義します。

```cpp
namespace {
enum : FNetworkRPCId {
  RPC_ServerApplyDamage = 101,
  RPC_MulticastPlayHitEffect = 102
};
}
```

既存RPCとの重複を避けるため、Componentまたは機能単位でID範囲を管理してください。

### 6.2 RPC登録

```cpp
MHealthComponent::MHealthComponent() {
  bReplicates = true;
  SetNetComponentName("HealthComponent");

  RegisterRPC(
      RPC_ServerApplyDamage,
      ENetRPCType::Server,
      this,
      &MHealthComponent::ServerApplyDamage
  );

  RegisterRPC(
      RPC_MulticastPlayHitEffect,
      ENetRPCType::Multicast,
      this,
      &MHealthComponent::MulticastPlayHitEffect
  );
}
```

### 6.3 RPC呼び出し

```cpp
void MHealthComponent::RequestDamage(float Damage) {
  InvokeRPC(
      RPC_ServerApplyDamage,
      ENetRPCType::Server,
      ENetPacketReliability::Reliable,
      Damage
  );
}
```

引数は `FNetBuffer` へ順番に書き込まれ、登録されたメソッドの引数順に復元されます。

### 6.4 Server RPC

クライアントからサーバーへ処理を要求します。

```cpp
void MHealthComponent::ServerApplyDamage(float Damage) {
  Damage = std::clamp(Damage, 0.0f, 1000.0f);

  CurrentHealth = std::max(0.0f, CurrentHealth - Damage);
  MarkReplicatedStateDirty();

  InvokeRPC(
      RPC_MulticastPlayHitEffect,
      ENetRPCType::Multicast,
      ENetPacketReliability::Reliable,
      CurrentHealth
  );
}
```

Server RPCの引数はクライアント入力として扱い、必ず検証してください。

### 6.5 Multicast RPC

サーバーから接続済みクライアントへイベントを配信します。

```cpp
void MHealthComponent::MulticastPlayHitEffect(float RemainingHealth) {
  PlayHitEffect(RemainingHealth);
}
```

永続状態はReplicated Propertyで同期し、Multicast RPCは一時的なイベントに使用します。

### 6.6 Client RPC

特定クライアント向け処理に使用します。対象Connectionの選択方法はActor/Replication System側のAPI契約に従ってください。

## 7. Reliability

| 種別 | 用途 |
|---|---|
| `Reliable` | 状態変更要求、購入、スポーン、重要イベント |
| `Unreliable` | 高頻度入力、補間用データ、一時的なエフェクト |

Reliableを高頻度で送信すると、再送待ちにより遅延が増える可能性があります。毎フレーム更新値は差分同期またはUnreliable送信を検討してください。

## 8. Actor同期とComponent同期

Replication SystemはActor単位で次の処理を扱います。

- Spawn
- State
- Destroy
- RPC
- Network ID割り当て
- Scene Travel

Component RPCはActor RPCパケット内の `ComponentNetworkId` を使用して対象Componentへ分岐されます。

Componentを同期する条件：

- 所有ActorがネットワークActorとして登録されている
- Componentの `bReplicates` が有効
- Component名またはNetwork IDが両端で対応する
- RPC IDと登録シグネチャが一致する

## 9. ENet / EOS P2P トランスポート

### 9.1 トランスポート種別

```cpp
enum class ENetworkTransportType : uint8_t {
  ENet,
  EOSP2P
};
```

上位層は共通NetworkManager/OnlinePlayManagerインターフェースを使用し、配送実装を切り替えます。

### 9.2 ENet

主に直接接続、LAN、IP/Portベースの開発テストに適します。

特徴：

- 接続先アドレスとポートを直接指定
- ローカル環境で検証しやすい
- EOSログインやLobbyを必要としない構成に適する

### 9.3 EOS P2P

Epic Online ServicesのP2P機能を利用します。

特徴：

- EOSユーザー識別子を利用
- Lobby検索・参加フローと統合可能
- NAT越えを含むインターネット接続向け
- EOS初期化、ログイン、Lobby状態の管理が必要

### 9.4 切り替え

```cpp
OnlinePlayManager& Online = OnlinePlayManager::GetInstance();

bool bConfigured = Online.ConfigureTransport(
    ENetworkTransportType::EOSP2P
);

if (!bConfigured) {
  // 現在のセッション状態、初期化状態、設定値を確認
}
```

接続開始前に選択します。接続中の変更は、セッション終了、Transport停止、状態ロールバックを伴う可能性があります。

## 10. 実装例

```cpp
class MNetworkHealthComponent : public MActorComponent {
 public:
  MNetworkHealthComponent();

  void RequestDamage(float Damage);

 private:
  void ServerApplyDamage(float Damage);
  void MulticastDamageEffect(float RemainingHealth);
  void OnRepHealth(float OldHealth);

  float Health = 100.0f;
};
```

```cpp
namespace {
enum : FNetworkRPCId {
  RPC_ServerApplyDamage = 201,
  RPC_MulticastDamageEffect = 202
};
}

MNetworkHealthComponent::MNetworkHealthComponent() {
  bReplicates = true;
  SetNetComponentName("NetworkHealthComponent");

  RegisterReplicatedProperty(
      &Health,
      this,
      &MNetworkHealthComponent::OnRepHealth
  );

  RegisterRPC(
      RPC_ServerApplyDamage,
      ENetRPCType::Server,
      this,
      &MNetworkHealthComponent::ServerApplyDamage
  );

  RegisterRPC(
      RPC_MulticastDamageEffect,
      ENetRPCType::Multicast,
      this,
      &MNetworkHealthComponent::MulticastDamageEffect
  );
}

void MNetworkHealthComponent::RequestDamage(float Damage) {
  InvokeRPC(
      RPC_ServerApplyDamage,
      ENetRPCType::Server,
      ENetPacketReliability::Reliable,
      Damage
  );
}

void MNetworkHealthComponent::ServerApplyDamage(float Damage) {
  Damage = std::clamp(Damage, 0.0f, 100.0f);
  Health = std::max(0.0f, Health - Damage);
  MarkReplicatedStateDirty();

  InvokeRPC(
      RPC_MulticastDamageEffect,
      ENetRPCType::Multicast,
      ENetPacketReliability::Reliable,
      Health
  );
}

void MNetworkHealthComponent::OnRepHealth(float OldHealth) {
  UpdateHealthUI(OldHealth, Health);
}

void MNetworkHealthComponent::MulticastDamageEffect(float RemainingHealth) {
  PlayDamageEffect(RemainingHealth);
}
```

## 11. セキュリティと整合性

- Clientから受け取るServer RPC引数は信用しない
- RPC側で範囲、権限、所有者、実行頻度を検証する
- クライアントから直接Replicated Propertyを確定させない
- 永続状態はサーバーを正とする
- Component名、RPC ID、引数順序をサーバーとクライアントで一致させる
- 異なるゲームバージョン間のプロトコル互換性を管理する
- 不正なPayloadを受信した場合は処理を中止する

## 12. デバッグチェックリスト

1. WorldのNetModeが期待値か
2. 所有ActorがReplication Systemへ登録されているか
3. Componentの `bReplicates` がtrueか
4. `SetNetComponentName()` が両端で一致するか
5. Replicated Property登録がコンストラクタで完了しているか
6. 値変更後に `MarkReplicatedStateDirty()` を呼んでいるか
7. RPC IDとRPC Typeが一致するか
8. RPC引数の型と順序が一致するか
9. Transportが起動済みか
10. EOSP2Pの場合、EOS初期化・ログイン・Lobby参加が完了しているか
