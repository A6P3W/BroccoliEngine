# BROCCOLI ENGINE

1. setup-vcpkg.md
2. 構成を指定し、エンジンのビルド
3. ソリューションビルド　

## Automation HTTP Server

Automation HTTP Serverは既定で無効です。localhost限定の状態取得APIを有効にする場合は、
Editorまたはゲームの起動時に`-automation`を指定します。

```text
Launcher.exe -automation
```

起動後は次のAPIを利用できます。

```text
GET http://127.0.0.1:39100/api/v1/state
```

レスポンス例:

```json
{
  "success": true,
  "data": {
    "sceneName": "BasicGameplay",
    "fps": 60.0,
    "paused": false,
    "worldAvailable": true,
    "actorCount": 23
  }
}
```

サーバーは`127.0.0.1`以外へbindしません。現時点でMCP BridgeおよびActor操作APIは
実装対象外です。
