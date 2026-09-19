# ゲームコードから Automation method を登録する

ゲームコードが使う登録 API は次の3マクロである。

- `REGISTER_AUTOMATION_METHOD(...)`
- `AUTOMATION_PARAMS(...)`
- `AUTOMATION_PARAM(Name, Description)`

ゲームコードは `AutomationMacros.h` を include する。

引数を持たない method は、メンバー関数と権限を登録する。

```cpp
#include "AutomationMacros.h"

REGISTER_AUTOMATION_METHOD(
    "open_door",
    "Opens the door when it is unlocked.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::OpenDoor
)
```

引数を持つ method では、`AUTOMATION_PARAM` をメンバー関数と同じ順序で並べる。登録時には引数の数がコンパイル時に検証される。

```cpp
REGISTER_AUTOMATION_METHOD(
    "set_locked",
    "Sets the door lock state.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::SetLocked,
    AUTOMATION_PARAMS(AUTOMATION_PARAM("locked", "New lock state."))
)
```

標準変換できない戻り値は、6番目の引数に Result Adapter を指定して JSON へ変換する。カンマを含むラムダは、全体を丸括弧で囲む。

```cpp
REGISTER_AUTOMATION_METHOD(
    "get_door_state",
    "Returns the current door state.",
    EAutomationPermission::ReadOnly,
    &ADoorActor::GetDoorState,
    AUTOMATION_PARAMS(),
    ([](const FDoorState& State) {
      return nlohmann::json{{"is_open", State.bIsOpen}, {"is_locked", State.bIsLocked}};
    })
)
```