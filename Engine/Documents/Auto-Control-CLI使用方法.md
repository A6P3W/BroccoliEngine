# Control Server を起動する

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

`broccoli.bat control` は対象実行ファイルのディレクトリだけを調べる。PID未指定なら最後にビルドした構成を使用する。PIDを指定した場合は、そのプロジェクトのDebug / Editor / Releaseの実行ファイルと照合するため、最後のビルドが別の構成でも接続できる。JSON の形式、PID、実行ファイルパス、ポート番号を検証し、条件を満たす登録だけを接続候補にする。

```powershell
broccoli.bat control state
broccoli.bat control actors
```

有効な候補が1つなら CLI はその Engine へ接続する。複数ある場合は PID を指定して実行する。

```powershell
broccoli.bat control --pid 12340 state
```

詳細は以下を使用。

```powershell
broccoli.bat control --help
```


## 非表示・Engine停止で起動する

描画コンテキストを維持したままウィンドウを非表示にし、初期レベルのロード・初期ActorのFlushが終わったところで停止できる。

```powershell
broccoli.bat run Debug --control --json -- --window-hidden --engine-update stopped --step-delta-time 0.016666667
```

`--json` はEngineプロセス本体の `pid`、`executable`、`configuration` を1行のJSONで返す。Debug / Releaseではパッケージの起動用EXEを経由せず、`Binaries` 内のEngine本体を同じ作業ディレクトリで起動する。このモードでは子プロセスの標準出力・標準エラーを切り離す。Engineログは `control logs` で取得する。起動完了は意味しない。返されたPIDの登録ファイルが現れ、`control --pid <PID> state` で `worldAvailable=true` を取得できるまで待つ。起動時停止では `completedEngineUpdates=0` のまま初期Actorの一覧を取得できる。

Engine起動引数（`run` の `--` より後に渡す）：

| 引数 | 内容 |
| --- | --- |
| `--window-hidden` | InitWindow前に非表示を設定。GPU・オフスクリーンバッファは利用可能 |
| `--engine-update running / stopped` | 自動更新の初期設定。既定はrunning |
| `--step-delta-time <秒>` | ステップ固定DeltaTime。既定は1/60秒。正の有限値かつ0.1秒以下 |

DeltaTime上限は通常更新の既存クランプ（0.1秒）に合わせている。再開直後の1更新にも固定DeltaTimeを使い、停止中の実時間をWorldに渡さない。

## Engine更新を停止・再開・ステップ実行する

以下のPIDは起動結果の値に置き換える。

```powershell
broccoli.bat control --pid 12340 stop-engine-update --wait
broccoli.bat control --pid 12340 state
broccoli.bat control --pid 12340 step-engine-update 10 --wait --wait-timeout 30
broccoli.bat control --pid 12340 start-engine-update
```

Engine停止中はcontrolとOSイベントのみ処理し、Plugin・通信・入力・World更新と描画を停止する。停止中の生成・破棄・Transform変更・公開メソッド呼び出しは即座に反映される。生成Actorは次のステップの更新対象となる。

Engine更新とWorld Simulationは独立している。`start_simulation` / `stop_simulation` の既存の意味は変わらず、Simulation無効でステップしても通常Actor、物理、タイマー等のSimulation対象は進まない。

ステップは停止中だけ受け付け、`count` は1〜1,000,000の整数。EngineUpdate正常完了を1回と数え、N回完了後は停止状態に戻る。描画完了は操作の完了条件に含まれない。

ステップ中は状態・Actor情報・ログ・操作状況を取得できる。Actor操作、公開メソッド呼び出し、Simulation変更、別step、controlからのレベル切替などの変更要求はHTTP 409 `CONFLICT` となる。`stop-engine-update` は実行中の更新完了後にステップを中断し、ステップ操作を `cancelled` にする。`completedCount` にはその更新も含まれる。ゲームロジックや通信によるシーン切替はステップ中も通常どおり実行し、残り回数を引き継ぐ。

Engine停止中はネットワークService・レプリケーションも停止する。外部の時間は進むため長時間停止で切断される場合がある。固定DeltaTimeは外部通信の到着順まで保証しない。

## 操作の受付と完了を区別する

`stop_engine_update`、`step_engine_update`、レベル切替は受付時に `operationId` を返す。`system-call` でも同じAPIを使える。

```powershell
broccoli.bat control --pid 12340 system-call step_engine_update --arguments '{"count":10}'
broccoli.bat control --pid 12340 operation-status 1
broccoli.bat control --pid 12340 operation-status 1 --wait
broccoli.bat control --pid 12340 system-call stop_engine_update --wait
```

操作には `pending / running / completed / cancelled / failed` の状態がある。結果には `operationId`、`kind`、`status`、`requestedCount`、`completedCount`、`startUpdate`、`endUpdate`、`reason` を含む。更新番号はApplicationの寿命を通じて増え、World切替でリセットしない。履歴はプロセス内に直近256件まで保持し、未完了操作は削除しない。不明または期限切れのIDは `INVALID_ARGUMENT` となる。

`--wait` はクライアントが操作完了を待つ指定。Engineを停止する指定ではない。既定の待機期限は30秒。`--wait-timeout` に正の有限秒数を指定できる。待機期限超過や通信タイムアウトはEngine内の操作失敗を意味しない。取得済みの同じ操作IDを問い合わせ、stepを再送しない。待機済みの操作が `failed` / `cancelled` ならCLIは結果JSONを出力して終了コード1を返す。

通常の停止要求は更新境界で確定し、stop操作の `completed` で停止完了を確認できる。終了要求は未完了操作を `cancelled` にして登録ファイルを削除する。

### stateの追加フィールド

| フィールド | 意味 |
| --- | --- |
| `engineUpdate` | 自動更新設定（running / stopped）。ステップ中もstopped |
| `engineStopped` | 更新実行中・ステップ実行中でない停止状態 |
| `stepping` / `stepDeltaTime` | ステップ操作の有無と固定DeltaTime |
| `completedEngineUpdates` | 正常完了したEngine更新の通算回数 |
| `sceneGeneration` | 成功したシーン切替回数（初期ロードを含む） |
| `activeOperationId` | ステップまたは停止操作ID。なければ空文字 |
| `levelOperationId` / `levelTransitionPending` | controlのレベル切替操作と完了待ちの有無 |

既存の `worldAvailable`、`simulating`、`actorCount`、`physics3DBodyCount` 等も引き続き取得できる。

## レベルロード直後に停止する

Engine実行中にレベル切替を要求する。`afterUpdate` の既定は `preserve`。`preserve` / `run` は実行継続、`stop` はロード・初期ActorのFlush・Editorバッファ設定完了後、新Worldの最初の更新前に停止する。

```powershell
broccoli.bat control --pid 12340 start-engine-update
broccoli.bat control --pid 12340 system-call open_level_by_path --arguments '{"levelPath":"Resources/SceneTest.BLevel.json","afterUpdate":"stop"}' --wait
```

`open_level_by_id` にも同じ `afterUpdate` を指定できる。Engine停止中・ステップ中のcontrolからの切替は拒否する。切替要求が未完了の間は、別のcontrol切替およびstopを競合として拒否する。ゲーム内で先に切替要求が置き換えられた場合、元の操作を `failed` とする。ロード失敗でも旧シーンと実行状態を維持し、操作に理由を返す。

## Python Client

```python
with ControlClient(Config) as Engine:
  Stop = Engine.stop_engine_update()
  Engine.wait_for_operation(Stop.OperationId)
  Step = Engine.step_engine_update(10)
  Result = Engine.wait_for_operation(Step.OperationId, TimeoutSeconds=30.0)
  assert Result.Status == "completed" and Result.CompletedCount == 10
  Engine.start_engine_update()
```

`get_operation_status(OperationId)` で再照会できる。待機は状態取得のみを繰り返し、step自体を再送しない。


## 更新制御の自動検証

DebugとEditorをビルドした後、次のテストを実行できる。

```powershell
pwsh -File Tests/EngineUpdateControl/Run-EngineUpdateControlTests.ps1
uv run --project Tools/Control --extra dev python Tools/Control/tests/live_engine_update_control.py
```

C++テストは更新境界・ステップ完了・中断・競合・レベル切替結果を検証する。実機スクリプトは `broccoli.bat run Debug --control --json` で非表示Engineを起動し、PID指定での状態取得、Actor操作、Simulation停止時と実行時の物理Actor位置、固定回数更新、中断、ロード後停止、失敗時の実行継続、正常終了を確認する。結果と取得ログは `Intermediate/EngineUpdateControlIntegration.json` に保存する。
