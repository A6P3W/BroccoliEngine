# Control Server を起動する

MCPは非推奨です。broccoli.bat controlの仕様を推奨します。

Control Server は Engine に `--control` を渡した場合だけ起動する。開発中は次のコマンドを使う。

```powershell
broccoli.bat run Debug --control
```

配布済みのゲームを直接起動する場合も、Engine の引数として `--control` を渡す。

```powershell
Publish\Debug\Binaries\Launcher.exe --control
```

Engine は `127.0.0.1:39100` から順に bind を試し、最初に確保できたポートを使う。空きポートを調べてから bind する処理は行わない。bind に成功したポートだけを使用するため、同一 PC で複数の Engine を起動してもポートは重複しない。

## 接続先を登録する

Engine は起動後、実行ファイルと同じディレクトリの `control/<pid>.json` に接続先を登録する。

```json
{
  "pid": 12340,
  "port": 39100
}
```

正常終了時には、Engine は自分の登録ファイルを削除する。

`broccoli.bat control` は対象実行ファイルのディレクトリだけを調べる。JSON の形式、PID、実行ファイルパス、ポート番号を検証し、条件を満たす登録だけを接続候補にする。

```powershell
broccoli.bat control state
broccoli.bat control actors
```

有効な候補が1つなら CLI はその Engine へ接続する。複数ある場合は PID を指定して実行する。

```powershell
broccoli.bat control --pid 12340 state
```