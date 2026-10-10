# ゲームコードから Control method を公開する

ゲームコードが使用するAPIは次の3マクロである。

- `CONTROL_METHOD(...)`
- `CONTROL_PARAMETER(...)`
- `REGISTER_CONTROL_CLASS(T)`

関数の宣言には`AutomationAnnotations.h`、クラスの登録には`ControlMacros.h`を使用する。

## 引数を持たないmethod

ヘッダーのメンバー関数宣言に`CONTROL_METHOD`を付ける。

```cpp
#include "AutomationAnnotations.h"

class ADoorActor : public AActor {
public:
    CONTROL_METHOD(
        .Name = "open_door",
        .Description = "Opens the door."
    )
    void OpenDoor();
};
```

## 引数を持つmethod

`CONTROL_PARAMETER`で引数名と説明を指定する。`Index`は0から始まる。

```cpp
CONTROL_METHOD(
    .Name = "set_locked",
    .Description = "Sets the door lock state."
)
CONTROL_PARAMETER(
    .Index = 0,
    .Name = "locked",
    .Description = "New lock state."
)
void SetLocked(bool bLocked);
```

`CONTROL_PARAMETER`を省略した引数は、C++の引数名をJSONのフィールド名として使用する。引数名を取得できない場合は明示的な指定が必要である。

## 戻り値の変換

標準変換できない戻り値は、`ResultAdapter`でJSONへ変換する。

```cpp
static nlohmann::json DoorStateToJson(
    const FDoorState& State
) {
    return {
        {"is_open", State.bIsOpen},
        {"is_locked", State.bIsLocked}
    };
}
```

```cpp
CONTROL_METHOD(
    .Name = "get_door_state",
    .Description = "Returns the current door state.",
    .ResultAdapter = ^^ADoorActor::DoorStateToJson
)
FDoorState GetDoorState() const;
```

`ResultAdapter`には名前付きの関数を指定する。ラムダ式は直接指定しない。

## クラスの登録

`.cpp`でクラス単位のControl登録を行う。

```cpp
#include "DoorActor.h"
#include "ControlMacros.h"

REGISTER_ACTOR(ADoorActor)
REGISTER_CONTROL_CLASS(ADoorActor)
```

Componentの場合は`REGISTER_COMPONENT(T)`を使用する。

```cpp
REGISTER_COMPONENT(MDoorComponent)
REGISTER_CONTROL_CLASS(MDoorComponent)
```

## Pluginからの登録

動的Pluginでは`REGISTER_CONTROL_CLASS`ではなく、`PluginContext`から登録する。

```cpp
bool OnLoad(PluginContext& Context) override {
    if (!Context.RegisterComponent<MDoorComponent>()) {
        return false;
    }
    return Context.RegisterControlClass<MDoorComponent>();
}
```

Pluginのアンロード時には、登録したControlメソッドが`ModuleOwner`単位で解除される。