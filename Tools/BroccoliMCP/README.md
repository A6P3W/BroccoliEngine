# BROCCOLI ENGINE MCP Bridge

安定後、リポジトリ分離されます。

BROCCOLI ENGINEのlocalhost限定Automation APIを、MCP Stdio ResourceとToolとして
公開するPython Bridgeです。読み取り専用Resource `game://state`、
`game://world/actors`、`game://logs/recent`と、Actor操作Tool `spawn_actor`、
`destroy_actor`、`set_actor_transform`、`invoke_actor_method`、System操作Tool
`execute_system_command`を提供します。

## 必要環境

- Windows x64
- uv
- Automation APIを含むBROCCOLI ENGINE

Python 3.11〜3.14をサポートし、3.14を推奨します。

BridgeはEngineプロセスを起動・終了しません。また、任意URL、任意HTTPメソッド、
シェル、ファイル操作をMCPへ公開しません。

## セットアップ

```powershell
Tools/BroccoliMCP/setup-mcp.bat
```
uvがプロジェクト用の`.venv`を内部的に作成・管理します。

MCP SDK 1.26.0と、Stdio互換を確認済みのAnyIO 4.10.0を明示的に固定しています。
依存関係を意図的に変更した場合だけ`uv lock`を実行し、更新された`uv.lock`を
コミットします。

## 起動

先にBROCCOLI ENGINEをAutomation有効で起動します。

```powershell
<engine-or-game-executable>.exe -automation
```

Bridge単体の起動確認:

```powershell
uv run --frozen python -m broccoli_mcp
```

起動時にEngineへ接続しないため、Engineを後から起動してもBridgeの再起動は不要です。
診断ログは`stderr`、MCP通信だけが`stdout`へ出力されます。

## MCPクライアント設定例

`(Get-Command uv).Source`で`uv.exe`の絶対パスを確認し、設定例のパスを実際の環境へ
置き換えてください。MCPクライアントからもuv経由でBridgeを起動します。

```json
{
  "mcpServers": {
    "broccoli-engine": {
      "command": "C:\\Users\\<user>\\AppData\\Local\\Microsoft\\WinGet\\Links\\uv.exe",
      "args": ["run", "--frozen", "python", "-m", "broccoli_mcp"],
      "cwd": "C:\\Projects\\BroccoliEngine\\Tools\\BroccoliMCP"
    }
  }
}
```

## 設定

コマンドライン引数は同名の環境変数より優先されます。

| 引数 | 環境変数 | 既定値 |
|---|---|---:|
| `--host` | `BROCCOLI_MCP_HOST` | `127.0.0.1` |
| `--port` | `BROCCOLI_MCP_PORT` | `39100` |
| `--connect-timeout` | `BROCCOLI_MCP_CONNECT_TIMEOUT` | `1.0` |
| `--read-timeout` | `BROCCOLI_MCP_READ_TIMEOUT` | `4.0` |
| `--log-level` | `BROCCOLI_MCP_LOG_LEVEL` | `INFO` |

セキュリティ上、`--host`には`127.0.0.1`以外を指定できません。

## テスト

```powershell
uv run --frozen --extra dev ruff check broccoli_mcp tests
uv run --frozen --extra dev ruff format --check broccoli_mcp tests
uv run --frozen --extra dev pytest
```

Pythonコードを変更した場合は、テスト前にRuffでlint修正と整形を行います。
Pythonのインデントはスペース2個です。

```powershell
uv run --frozen --extra dev ruff check --fix broccoli_mcp tests
uv run --frozen --extra dev ruff format broccoli_mcp tests
```

実Engineとの結合確認では、MCPクライアントから`game://state`と
`game://world/actors`を読み取り、Engine停止、起動、再起動の各状態で、Bridgeを
再起動せず結果または明確な接続エラーが返ることを確認してください。

Engineを`-automation`付きで起動した状態では、次のライブ結合スクリプトでMCP Stdio
初期化、Resource一覧、State、World Actor、直近ログの読取をまとめて確認できます。
LevelStarterを起動している場合は、登録済み`get_status` Actor MethodのMCP Tool実行も
確認します。また、`pause_game`と`resume_game`を実行し、停止中もState Resourceが
応答することを確認します。

```powershell
uv run --frozen python .\tests\live_engine_integration.py
```

Actor操作がHTTPタイムアウトになる時点ですでにメインスレッドで処理を開始していた場合、
Bridgeにはタイムアウトが返っても操作自体は完了することがあります。タイムアウト後は
Resourceで現在状態を再確認してください。

## 主なエラー

- `ENGINE_UNAVAILABLE`: Engineが未起動、`-automation`なし、またはポート不一致
- `ENGINE_TIMEOUT`: Engineが設定時間内に応答しなかった
- `INVALID_ENGINE_RESPONSE`: EngineとBridgeのJSON契約が一致しない
- Engine固有コード: `WORLD_NOT_AVAILABLE`、`ACTOR_NOT_FOUND`、`REQUEST_TIMEOUT`など
