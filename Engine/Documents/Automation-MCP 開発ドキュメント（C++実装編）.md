# BroccoliEngine Automation & MCP 開発ガイド（C++実装編）

BroccoliEngine の Automation は、C++ の Actor / Component メソッドと Engine の
System Command を localhost 限定 HTTP API へ公開し、BroccoliMCP から操作できるようにします。
HTTP Server は起動引数 `-automation` が指定された場合だけ有効になります。

## 1. 公開登録 API

ゲームコードが使用する正式な登録 API は次の3マクロです。

- `REGISTER_AUTOMATION_METHOD(...)`
- `AUTOMATION_PARAMS(...)`
- `AUTOMATION_PARAM(Name, Description)`

Registry や AutoRegistrar の具体型は Engine 内部実装です。ゲームコードから直接操作せず、
`AutomationMacros.h` のみを include してください。マクロはメンバー関数の所有型から Actor と
Component を自動判定します。

### 引数なし

```cpp
#include "AutomationMacros.h"

REGISTER_AUTOMATION_METHOD(
    "open_door",
    "Opens the door when it is unlocked.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::OpenDoor
)
```

### 引数あり

`AUTOMATION_PARAM` の並びはメンバー関数の引数順と一致させます。型と個数は C++20 の
template 制約によりコンパイル時に検証されます。

```cpp
REGISTER_AUTOMATION_METHOD(
    "set_locked",
    "Sets the door lock state.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::SetLocked,
    AUTOMATION_PARAMS(AUTOMATION_PARAM("locked", "New lock state."))
)
```

### Result Adapter

標準変換できない戻り値や、HTTP 応答用に整形したい戻り値には6番目の引数として
Result Adapter を指定します。

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

カンマを含むインラインラムダは、上の例のように全体を丸括弧で囲みます。

## 2. 権限

| 権限 | 用途 |
|---|---|
| `ReadOnly` | 状態を変更しない Actor / Component メソッド |
| `WorldMutation` | World 内の状態を変更する Actor / Component メソッド |
| `SystemMutation` | Engine の System Command |
| `Dangerous` | HTTP Automation では許可されない操作 |

Actor / Component endpoint は `ReadOnly` と `WorldMutation` のみを公開します。
System Command endpoint は `SystemMutation` のみを実行します。

## 3. HTTP API

既定 URL は `http://127.0.0.1:39100/api/v1` です。

| Method | Path | 機能 |
|---|---|---|
| GET | `/state` | Scene、FPS、Pause、Actor 数 |
| GET | `/logs/recent` | 直近ログ |
| GET | `/actor-classes` | 登録済み Actor class |
| GET | `/actor-classes/{className}/methods` | Class の Automation method |
| GET | `/levels` | 登録済み Level |
| GET/POST | `/world/actors` | Actor 一覧・生成 |
| GET/DELETE | `/world/actors/{actorId}` | Actor 取得・破棄 |
| PATCH | `/world/actors/{actorId}/transform` | Transform 更新 |
| GET | `/world/actors/{actorId}/components` | Component 一覧 |
| GET | `/world/actors/{actorId}/methods` | Actor method 一覧 |
| POST | `/world/actors/{actorId}/methods/{methodName}` | Actor method 実行 |
| GET | `/world/actors/{actorId}/components/{componentId}/methods` | Component method 一覧 |
| POST | `/world/actors/{actorId}/components/{componentId}/methods/{methodName}` | Component method実行 |
| GET | `/system/commands` | System Command 一覧 |
| POST | `/system/commands/{commandName}` | System Command 実行 |

成功応答は `{"success": true, "data": ...}`、失敗応答は
`{"success": false, "error": {"code": "...", "message": "..."}}` です。

主な Error Code は `INVALID_REQUEST`、`INVALID_JSON`、`INVALID_ARGUMENT`、
`REQUEST_TOO_LARGE`、`WORLD_NOT_AVAILABLE`、`ACTOR_NOT_FOUND`、
`CLASS_NOT_REGISTERED`、`METHOD_NOT_REGISTERED`、`COMMAND_NOT_REGISTERED`、
`PERMISSION_DENIED`、`REQUEST_TIMEOUT`、`ENGINE_SHUTTING_DOWN`、`INTERNAL_ERROR` です。

## 4. 内部構成

- `Runtime`: Subsystem、Command Queue、Pause state、State Provider、Built-in Command
- `Registration`: Registry、登録検証、AutoRegistrar の callback 管理
- `World`: World/Discovery Adapter と Transport 非依存 DTO
- `Transport/Http`: routing、request/response 変換、機能別 Controller

ゲームモジュールで静的生成される登録 Token は、`Public/Detail` の型消去 Bridge を通して
Engine 内部 Registry へ登録されます。具体的な Registry 型が公開 ABI を横断することはありません。

## 5. ビルドとテスト

```powershell
broccoli.bat build debug
broccoli.bat build editor
broccoli.bat build release
```

Debug 版を Automation 有効で起動します。

```powershell
broccoli.bat run debug -- -automation
```

別のターミナルから MCP の単体テストと Live integration を実行します。

```powershell
cd Tools/BroccoliMCP
uv run --frozen --extra dev pytest
uv run --frozen python .\tests\live_engine_integration.py
```

Live integration は State、Discovery、Actor の生成・更新・破棄、Actor/Component method、
System Command、Pause/Resume、Recent Logs を検証します。
