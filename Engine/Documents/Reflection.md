# Actor Reflection

GCC 16 以上の C++26 Reflection (`-freflection`) を使用します。公開されるのは、`EDITOR_PROPERTY(...)` を付けた public の非 static データメンバだけです。Property 型はメンバの実型から決まります。通常ビルドでは共通 Annotation に展開され、IntelliSense 時には annotation 全体が除外されます。未注釈メンバと Transform は従来の経路を使います。

```cpp
#include "ReflectionGenerator.h"

class AEnemy : public AActor {
  DEFINE_ACTOR_CLASS(AEnemy)

 protected:
  virtual void OnSpeedChanged(float OldValue);

 public:
  EDITOR_PROPERTY(
      .OnChanged = ^^AEnemy::OnSpeedChanged,
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

Callback は Annotation より前に宣言してください。Metadata が不要なら `EDITOR_PROPERTY()` と書きます。対応するメンバ型は `bool`、`int`、`float`、`std::string`、`FVector2D`、`FVector3D` です。

| メンバ型 | 指定できる Metadata |
| --- | --- |
| `bool`、`FVector2D`、`FVector3D` | `OnChanged` |
| `int`、`float` | `OnChanged`、`Min`、`Max`、`SliderMin`、`SliderMax` |
| `std::string` | `OnChanged`、`MaxLength` |

数値 Metadata は `int` と `float` のどちらにも指定できます。`int` では整数かつ表現範囲内の有限値、`float` では表現範囲内の有限値が必要です。未対応のメンバ型、不適切な Metadata、逆転した範囲、Callback の引数型不一致はコンパイル時に拒否されます。

`REGISTER_ACTOR` は Actor と Reflection の両方を登録し、C++26 Reflection で直接の基底クラスを取得します。派生 Actor の親 Reflection 登録は必要に応じて先に行われます。名前の重複や非 public への Annotation は登録・コンパイル時に拒否されます。親の virtual Callback は派生クラスで override できます。

Plugin は `OnLoad(PluginContext& Context)` の中で `Context.RegisterActor<T, Base>()` を使用してください。Host は Plugin の登録情報を DLL unload 前に解除し、生存 Actor がいる間は unload を延期します。Plugin Actor に従来の静的 `REGISTER_ACTOR` は使用しません。

`FProperty::Set` は入力型、UTF-8 の Unicode scalar 数、有限値を検査します。Min/Max は実値を clamp し、SliderMin/SliderMax は Inspector 操作範囲だけに使います。値が変わったときだけ Callback を 1 回呼び、再入は拒否します。Callback が例外を投げた場合、代入済みの値を維持してログに記録し、`false` を返します。
