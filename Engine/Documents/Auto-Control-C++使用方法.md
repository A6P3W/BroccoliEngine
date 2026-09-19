# ゲームコードから Control method を公開する

ゲームコードが使う登録 API は次の3マクロである。

- `CONTROL_METHOD(...)`
- `CONTROL_PARAMETERS(...)`
- `CONTROL_PARAMETER(Name, Description)`

ゲームコードは `ControlMacros.h` を include する。

引数を持たない method は、メンバー関数を登録する。

```cpp
#include "ControlMacros.h"

CONTROL_METHOD(
    "open_door",
    "Opens the door when it is unlocked.",
    &ADoorActor::OpenDoor
)
```

引数を持つ method では、`CONTROL_PARAMETER` をメンバー関数と同じ順序で並べる。登録時には引数の数がコンパイル時に検証される。

```cpp
CONTROL_METHOD(
    "set_locked",
    "Sets the door lock state.",
    &ADoorActor::SetLocked,
    CONTROL_PARAMETERS(CONTROL_PARAMETER("locked", "New lock state."))
)
```

標準変換できない戻り値は、6番目の引数に Result Adapter を指定して JSON へ変換する。カンマを含むラムダは、全体を丸括弧で囲む。

```cpp
CONTROL_METHOD(
    "get_door_state",
    "Returns the current door state.",
    &ADoorActor::GetDoorState,
    CONTROL_PARAMETERS(),
    ([](const FDoorState& State) {
      return nlohmann::json{{"is_open", State.bIsOpen}, {"is_locked", State.bIsLocked}};
    })
)
```
