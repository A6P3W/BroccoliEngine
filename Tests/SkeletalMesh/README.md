# スケルタルメッシュAPIテスト

リポジトリルートでDebugをビルドし、Controlを有効にして起動する。

```powershell
./broccoli.bat build Debug
./broccoli.bat run Debug --control
Tools/Build/.venv/Scripts/python.exe Tests/SkeletalMesh/RunControlTests.py
```

複数プロセスを起動している場合はテストスクリプトに `--pid <PID>` を指定する。
Control CLIは最後にビルドした構成を接続先として使用するため、
Editorをビルドした後はDebugをビルドしてから実行する。

スクリプトは `Game/Animation3DTest.BLevel` を開き、Survey/Walk/Runを再生する
3体のActorの状態を取得する。続いて `run_tests` と `test_fixtures` を呼ぶ。
各ケースの戻り値を検査し、さらにActorの生成・再生状態取得・破棄を12回繰り返す。
失敗があれば終了コードを非ゼロにする。
結果は `.tmp/skeletal-results.json` に保存する。

生成素材は配布済みDebugリソース内の `SkeletalFixtures` に置く。
元データはコミット済みのFox.glbで、アニメーションなし、スキンなし、
空・重複名、単一フレーム、Unicode名・大文字拡張子、外部バッファ付きglTFを生成する。
元のFox.glbは変更しない。BLevelの保存復元テストも同じ生成先を使う。

テストは再生状態、時刻、戻り値、ハンドルの有効性、反射プロパティ復元を確認する。
画面上の姿勢、材質、テクスチャはこのスクリプトの判定対象に含まれない。

## raylib 6.0の調査結果

使用中のraylibではモデルの骨情報は `Model::skeleton` にあり、
`ModelAnimation` は `boneCount`、`keyframeCount`、`keyframePoses` を持つ。
`IsModelAnimationValid` は骨数だけを比較するため、クリップは同じパスから取得する。

`UnloadModel` はメッシュ・マテリアル配列・骨情報を解放する一方で、
マテリアルのテクスチャと `currentPose` / `boneMatrices` を解放しない。
固有モデルの解放時には、既定テクスチャを除外してテクスチャIDを重複排除し、
姿勢配列も解放する。

参照: [raylib 6.0 rmodels.c](https://github.com/raysan5/raylib/blob/6.0/src/rmodels.c)
