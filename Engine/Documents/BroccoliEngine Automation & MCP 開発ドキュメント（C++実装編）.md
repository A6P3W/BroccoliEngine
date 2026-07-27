# BroccoliEngine Automation & MCP 開発ドキュメント（C++実装編）

BroccoliEngineの自動化システムは、C++側で定義したActorやComponentのメソッド、あるいはグローバルなシステムコマンドを登録マクロによって自動化サーバーに公開します。

## 1. アクターメソッドの登録（Actor Methods）

アクター（`AActor` の派生クラス）が持つメンバー関数を外部から呼び出せるように公開するには、自動生成/登録システム用マクロを使用します。

### 1.1 引数・戻り値なしのシンプルな登録

最も基本的なケースとして、引数がなく戻り値が `void` のメンバー関数を公開する例です。

#### ヘッダーファイル（`MyPawn.h` 等）

```cpp
#pragma once
#include "Actor.h"
#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API AMyPawn : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AMyPawn)
  AMyPawn();

  // 公開したいメソッド
  void ResetStatus();
};
```

#### ソースファイル（`MyPawn.cpp` 等）

```cpp
#include "MyPawn.h"
#include "AutomationRegistryHelper.h" // 登録用ヘルパーのインクルード

REGISTER_ACTOR(AMyPawn)

AMyPawn::AMyPawn() {
  // コンストラクタ処理
}

void AMyPawn::ResetStatus() {
  // 内部状態のリセット処理
}

// 自動化メソッドの登録
REGISTER_AUTOMATION_METHOD(
    "reset_status",                              // 外部公開名 (スネークケース / 小文字開始推奨)
    "Resets the pawn's core status to default.", // メソッドの説明
    EAutomationPermission::WorldMutation,        // 実行権限 (ReadOnly / WorldMutation など)
    &AMyPawn::ResetStatus                        // メンバー関数ポインタ
)
```

### 1.2 引数を持つメソッドの登録

引数を持つメソッドを登録する場合、`AUTOMATION_PARAMS` および `AUTOMATION_PARAM` マクロを用いてメタデータ（名前、説明、検証用の型情報）を指定します。

#### C++メソッドの追加

```cpp
// AMyPawn クラス内
void AddEnergy(float Amount, int BaseMultiplier);
```

#### メソッドの登録

```cpp
REGISTER_AUTOMATION_METHOD(
    "add_energy",
    "Adds energy to the pawn with a multiplier.",
    EAutomationPermission::WorldMutation,
    &AMyPawn::AddEnergy,
    AUTOMATION_PARAMS(
        AUTOMATION_PARAM("amount", "The base amount of energy to add."),
        AUTOMATION_PARAM("baseMultiplier", "The multiplier applied to the energy amount.")
    )
)
```

- **引数型の一致**: `AUTOMATION_PARAMS` で指定した引数の順序と型は、対象のメンバー関数のシグネチャと正確に一致している必要があります。
    
- **サポートされている型**: `float`, `double`, `bool`, `int`（各種符号あり/なし整数）, `std::string`, `FVector2D` 等が標準でJSON変換サポートされています。
    

### 1.3 戻り値を持つメソッド

戻り値が存在する場合、自動化システムが自動的にJSON形式にシリアライズしてクライアントに返却します。

#### C++メソッドの追加

```cpp
// AMyPawn クラス内
float GetCurrentHealth() const;
```

#### メソッドの登録

```cpp
REGISTER_AUTOMATION_METHOD(
    "get_current_health",
    "Returns the current health value of the pawn.",
    EAutomationPermission::ReadOnly,
    &AMyPawn::GetCurrentHealth
)
```

## 2. コンポーネントメソッドの登録（Component Methods）

`MActorComponent` の派生クラス（例: `MForceFieldComponent` や自作コンポーネント）のメソッドもアクターと同様に登録できます。

コンポーネントを定義しているクラスで `REGISTER_AUTOMATION_COMPONENT_METHODS(ClassName)` マクロを指定し、以下のように登録します。

#### コンポーネントクラスでの登録例

```cpp
// MyComponent.cpp 内
#include "MyComponent.h"
#include "AutomationRegistryHelper.h"

REGISTER_AUTOMATION_METHOD(
    "set_active",
    "Enables or disables the component.",
    EAutomationPermission::WorldMutation,
    &UMyComponent::SetActive,
    AUTOMATION_PARAMS(AUTOMATION_PARAM("active", "True to enable, false to disable."))
)
```

## 3. 高度なカスタマイズ：結果アダプター（Result Adapter）

戻り値のオブジェクトをそのまま自動でシリアライズできない場合や、戻り値のフォーマットをC++側で細かく整形したい場合は、結果アダプター（Result Adapter）関数を登録時に指定できます。

```cpp
// 任意の戻り値変換用ラムダ
auto HealthResultAdapter = [](const AMyPawn& Pawn, float OriginalResult) {
  return nlohmann::json{
      {"health", OriginalResult},
      {"isDead", OriginalResult <= 0.0f},
      {"instance", Pawn.GetInstanceName()}
  };
};

REGISTER_AUTOMATION_METHOD_6(
    "get_health_detailed",
    "Returns detailed health information.",
    EAutomationPermission::ReadOnly,
    &AMyPawn::GetCurrentHealth,
    AUTOMATION_PARAMS(), // 引数なし
    HealthResultAdapter  // 変換用アダプター
)
```

## 4. テスト方法

### 4.1 実行と自動化サーバーの有効化

自動化HTTPサーバーは、起動オプション `-automation` を付与したときのみ起動します。

```bash
# エディタまたはバイナリを起動してポート39100を開く
Launcher.exe -automation
```

### 4.2 動作確認（ブラウザまたはcurl）

以下のURLにアクセスしてJSON応答が返ってくるか確認します。

```http
GET http://127.0.0.1:39100/api/v1/state
```