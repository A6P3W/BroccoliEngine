# BROCCOLI ENGINE MCP Bridge

MCP Bridge は、BROCCOLI ENGINE の Control HTTP API を MCP Stdio へ接続する Python プロセスである。Bridge 自身は HTTP API の呼び出しだけを担当する。

## Bridge を起動する

Bridge のセットアップはリポジトリルートから実行する。

```powershell
Tools/BroccoliMCP/setup-mcp.bat
```

Bridge 単体の起動確認は次のコマンドで行う。

```powershell
cd Tools/BroccoliMCP
uv run --frozen python -m broccoli_mcp
```

Bridge は起動時に Engine へ接続しない。Engine を後から起動しても Bridge を再起動する必要はない。診断ログは `stderr`、MCP 通信は `stdout` へ出力する。

## MCP クライアントを設定する

`(Get-Command uv).Source` で `uv.exe` の絶対パスを確認し、設定例のパスを実際の環境へ置き換える。

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