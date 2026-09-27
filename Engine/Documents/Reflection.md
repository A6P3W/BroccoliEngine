# Actor Reflection

GCC 16 以上の C++26 Reflection (`-freflection`) を使用します。公開されるのは、`[[=...]]` を付けた public の非 static データメンバだけです。未注釈メンバと Transform は従来の経路を使います。

```cpp
#include "ReflectionGenerator.h"

class AEnemy : public AActor {
  DEFINE_ACTOR_CLASS(AEnemy)

 protected:
  virtual void OnSpeedChanged(float OldValue);

 public:
  [[=FFloatEditorProperty{
      .Base = {.OnChanged = ^^AEnemy::OnSpeedChanged},
      .Min = 0.0f,
      .Max = 1000.0f,
      .SliderMin = 0.0f,
      .SliderMax = 100.0f}]]
  float Speed = 0.0f;
};

// AEnemy.cpp
REGISTER_ACTOR(AEnemy)
```

Callback は Annotation より前に宣言してください。GCC 16 の Annotation 値は structural type を要するため、数値や文字数の任意項目には `TAnnotationOptional<T>` を使っています。指定子 `.Min = 0.0f` などは従来の設計例と同じです。

`REGISTER_ACTOR` は Actor と Reflection の両方を登録し、C++26 Reflection で直接の基底クラスを取得します。派生 Actor の親 Reflection 登録は必要に応じて先に行われます。名前の重複、非 public への Annotation、型不一致、不正な制約や Callback シグネチャは登録・コンパイル時に拒否されます。親の virtual Callback は派生クラスで override できます。

Plugin は `OnLoad(PluginContext& Context)` の中で `Context.RegisterActor<T, Base>()` を使用してください。Host は Plugin の登録情報を DLL unload 前に解除し、生存 Actor がいる間は unload を延期します。Plugin Actor に従来の静的 `REGISTER_ACTOR` は使用しません。

`FProperty::Set` は入力型、UTF-8 の Unicode scalar 数、有限値を検査します。Min/Max は実値を clamp し、SliderMin/SliderMax は Inspector 操作範囲だけに使います。値が変わったときだけ Callback を 1 回呼び、再入は拒否します。Callback が例外を投げた場合、代入済みの値を維持してログに記録し、`false` を返します。
