# ゲームコードから Control method を公開する

Actor / Component の header で `AutomationAnnotations.h` を include し、公開する member function の宣言に `CONTROL_METHOD` を付ける。cpp では `ControlMacros.h` を include して `REGISTER_CONTROL_CLASS(T)` を一度だけ記述する。Actor の場合は `REGISTER_ACTOR(T)` も必要である。

```cpp
class ADoorActor : public AActor {
 public:
  CONTROL_METHOD(.Name = "open_door", .Description = "Opens the door.")
  void OpenDoor();

  CONTROL_METHOD(.Name = "set_locked", .Description = "Sets the lock state.")
  CONTROL_PARAMETER(.Index = 0, .Name = "locked", .Description = "New lock state.")
  void SetLocked(bool bLocked);
};
```

```cpp
#include "ControlMacros.h"

REGISTER_ACTOR(ADoorActor)
REGISTER_CONTROL_CLASS(ADoorActor)
```

Parameter metadata は同じ関数宣言に複数付けられる。`Index` は 0 から始まる。省略した parameter は C++ の識別子を JSON field name に使い、description は空になる。名前のない parameter には metadata が必須である。`std::optional<T>` は従来どおり schema の `required` から除外される。

標準の JSON return converter が扱わない型には、名前付き static / free function を Result Adapter として指定する。Adapter は JSON に変換できる値を返す。

```cpp
class ADoorActor : public AActor {
 private:
  static nlohmann::json DoorStateToJson(const FDoorState& State);

 public:
  CONTROL_METHOD(
      .Name = "get_door_state",
      .Description = "Returns the current door state.",
      .ResultAdapter = ^^ADoorActor::DoorStateToJson
  )
  FDoorState GetDoorState() const;
};
```

Plugin は Actor / Component の型登録後に `PluginContext::RegisterControlClass<T>()` を呼ぶ。Plugin unload 時には同じ ModuleOwner に属する Control method が削除される。
