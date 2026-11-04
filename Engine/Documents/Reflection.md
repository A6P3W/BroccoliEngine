# Actor / Component Reflection

Component も `EDITOR_PROPERTY(...)` を使用します。`REGISTER_COMPONENT` は生成 Factory と
Reflection class を同時に登録し、基底 Component の Reflection class を再帰的に登録します。

```cpp
class MHealthComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MHealthComponent)

 private:
  EDITOR_PROPERTY(.Min = 0)
  int Health = 100;
};

// MHealthComponent.cpp
REGISTER_COMPONENT(MHealthComponent)
```

Inspector は Actor と所有 Component の Property を同じ `FProperty::Get/Set(void*)` で
描画します。`REGISTER_COMPONENT(MCamera3DComponent, .AllowMultiple = false)` のように
同一 Actor 内での Editor 追加を制限できます。`EditorAddable = false` は候補一覧から
除外します。Plugin Component は `PluginContext::RegisterComponent<T>()` で登録します。

Level v4 は `components` 配列に `name`、`class`、`source`、`properties` を保存します。
`NewObject<T>(this, "Health")` で作った native Component だけが Property override の
保存対象になります。無名の native Component は生成順に依存するため対象外です。
Registry から追加した Instance Component は自動名を含めて保存・復元されます。
`ComponentName` は `FComponentId` と `NetComponentName` から独立しています。

`EDITOR_PROPERTY(...)` を付けた非 static データメンバは、C++ のアクセス修飾子に関係なく Editor と Level 保存へ公開されます。Property 型はメンバの実型から決まります。

```cpp
#include "ReflectionGenerator.h"

class AEnemy : public AActor {
  DEFINE_ACTOR_CLASS(AEnemy)

 protected:
  virtual void OnSpeedChanged(float OldValue);

 private:
  EDITOR_PROPERTY(
      .OnEditorChanged = ^^AEnemy::OnSpeedChanged,
      .Min = 0.0f,
      .Max = 1000.0f,
      .SliderMin = 0.0f,
      .SliderMax = 100.0f
  )
  float Speed = 0.0f;

  EDITOR_PROPERTY(.Min = 0, .Max = 100)
  int Level = 0;

  EDITOR_PROPERTY()
  bool Enabled = true;

  EDITOR_PROPERTY(.MaxLength = 32)
  std::string DisplayName = "Enemy";
};

// AEnemy.cpp
REGISTER_ACTOR(AEnemy)
```

実際の利用例として `Engine/EngineSide/MovingPlatformActor.h` の `AMovingPlatformActor` があります。Inspector で `Speed`、`Distance`、`Direction`、`Enabled` を編集でき、開始位置から指定方向へ指定距離だけ進み、`Speed` に従って往復します。4つの値は private のまま `EDITOR_PROPERTY` で公開しています。`ATintTestActor` は6種類の対応型を確認する例として残しています。

Callback は Annotation より前に宣言してください。Metadata が不要なら `EDITOR_PROPERTY()` と書きます。対応するメンバ型は `bool`、`int`、`float`、`std::string`、`FVector2D`、`FVector3D`、`FPath` です。

`FPath` は UTF-8 の Resource 仮想パスを表します。`Textures/A.png` は `/Game/Textures/A.png` に正規化され、`/Engine/...` も利用できます。Resource Root 外の絶対パスは拒否されます。Inspector には入力欄とファイル選択ボタンが表示されます。フィルタ未指定時は Any File です。

```cpp
void OnImagePathChanged(FPath OldValue);
EDITOR_PROPERTY(.OnEditorChanged = ^^ASprite2DActor::OnImagePathChanged,
                .PathFilter = EPathFilter::Image)
FPath ImagePath;
```

`PathFilter` は `EPathFilter::AnyFile`、`EPathFilter::Image`（png、jpg、jpeg、bmp）、`EPathFilter::Model`（glb、gltf）から選びます。Level JSON では仮想パスを文字列として保存します。旧データのプレフィックスなし相対パスは読み込み時に `/Game/` へ正規化されます。
`ASprite2DActor` と `ASprite3DActor` の `SetImagePath` / `GetImagePath`、`AStaticMeshActor::SetModelPath` / `GetModelPath` の公開 API も `FPath` を使用します。両Sprite Actorの `ImagePath` は `EPathFilter::Image` でInspectorに公開され、Level JSONへ仮想Pathとして保存されます。文字列から渡す場合は `FPath("Textures/A.png")` を生成してください。
`ResourceManager::LoadResourceGraph` / `LoadResourceModel`、`FSoundManager::GetMasterHandle` / `PlaySE` / `PlayBGM`、`MSoundComponent::PlaySE` / `PlayBGM` も `FPath` を受け取ります。ファイル選択ダイアログの OS パスは `FPath` に変換してから渡してください。

Editor Property は `private` を標準とします。派生クラスから直接アクセスする必要があれば `protected`、外部コードからの直接アクセスが必要なら `public` を選びます。どのアクセス修飾子でも `EDITOR_PROPERTY` があれば Inspector と Level JSON の対象になり、付けなければ対象になりません。Annotation は通常の C++ アクセス制御を変更しません。Inspector と LevelSerializer は `FProperty::Get/Set` から値を操作します。

| メンバ型 | 指定できる Metadata |
| --- | --- |
| `bool`、`FVector2D`、`FVector3D` | `OnEditorChanged` |
| `int`、`float` | `OnEditorChanged`、`Min`、`Max`、`SliderMin`、`SliderMax` |
| `std::string` | `OnEditorChanged`、`MaxLength` |
| `FPath` | `OnEditorChanged`、`PathFilter` |

数値 Metadata は `int` と `float` のどちらにも指定できます。`int` では整数かつ表現範囲内の有限値、`float` では表現範囲内の有限値が必要です。未対応のメンバ型、不適切な Metadata、逆転した範囲、Callback の引数型不一致はコンパイル時に拒否されます。

`REGISTER_ACTOR` は Actor と Reflection の両方を登録し、C++26 Reflection で直接の基底クラスを取得します。派生 Actor の親 Reflection 登録は必要に応じて先に行われます。名前の重複は登録時に拒否されます。親の virtual Callback は派生クラスで override できます。

Plugin は `OnLoad(PluginContext& Context)` の中で `Context.RegisterActor<T, Base>()` を使用してください。Host は Plugin の登録情報を DLL unload 前に解除し、生存 Actor がいる間は unload を延期します。Plugin Actor に従来の静的 `REGISTER_ACTOR` は使用しません。

`FProperty::Set` は入力型、UTF-8 の Unicode scalar 数、有限値を検査します。Min/Max は実値を clamp し、SliderMin/SliderMax は Inspector 操作範囲だけに使います。値が変わったときだけ Callback を 1 回呼び、再入は拒否します。Callback が例外を投げた場合、代入済みの値を維持してログに記録し、`false` を返します。
