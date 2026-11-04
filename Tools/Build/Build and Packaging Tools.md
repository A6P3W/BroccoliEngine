# BroccoliEngine Build and Packaging Tools

Python >=3.11,<3.15 と uv を使用します。`broccoli.bat --help` で CLI を確認できます。

## ビルドと配布

```powershell
broccoli.bat build Debug
broccoli.bat build Editor
broccoli.bat build Release
```

`BroccoliProjectBuild_<Configuration>` が Game と全有効 Plugin をビルドします。
成功後に Build CLI が `plugins.json` 生成 → prepare-output → stage-runtime → verify-runtime → package-runtime → verify-runtime を実行します。
Game の POST_BUILD では配布物を作りません。Plugin のみ変更されたビルドでも配布物を更新します。
Editor は実行用 Stage と DLL 検証のみ行い、Package は作成しません。

`.broccoli-project.json` の plugins[].name / configurations が設定元です。
`generate-plugins` は Intermediate/Generated/Plugins.cmake を生成します。Public ヘッダーは生成しません。
`configurations: []` では Source/Public の include path だけを追加し、Plugin の Build.cmake や SDK は評価しません。
Game は有効構成のみ Plugin DLL に直接リンクします。呼び出しは `BROCCOLI_WITH_<PLUGIN名の大文字>` で構成別に除外できます。

```json
{"schema_version": 1, "required_plugins": ["EOSPlugin", "ExamplePlugin"]}
```

構成ごとに実行ファイル直下へ単一 plugins.json を配置します。Plugin なしでも空配列を生成します。
欠落・不正 schema version・不正名・重複名は検証と起動を失敗させます。
DLL 本体と依存 DLL は実行ファイル直下に各1つ配置し、Plugin サブディレクトリや個別 manifest は配置しません。
依存 DLL のパスは Plugin target の `BROCCOLI_RUNTIME_DEPENDENCIES` から構成別 runtime metadata に書き出します。
Build Tool には SDK 固有の引数やリソース処理はありません。

```text
Bin/x64/Debug/
  Launcher-game.exe
  BroccoliEngine.dll
  plugins.json
  ExamplePlugin.dll
  EOSPlugin.dll                  # 有効構成のみ
  EOSSDK-Win64-Shipping.dll     # EOSPlugin の依存
Publish/Debug/
  Launcher.exe                  # Bootstrap
  Binaries/                     # Game / DLL / plugins.json
  Resources/                    # Engine / Game resources
```

Stage は `.plugin-artifacts.json` に記録された管理対象の不要 DLL だけを削除し、ユーザーの DLL を維持します。
Publish はビルドごとに再生成します。Saved 内の実行時データをコピーしません。

## 個別コマンド

- `prepare-output --output-dir`: Logs / Resources / Log.txt の削除。ビルド成果物と Saved は維持。
- `stage-runtime`: Engine / Game resources、Engine DLL、MinGW runtime、Plugin 依存 DLL の配置。Editor はリソース変換を省略。
- `package-runtime`: Game、Bootstrap、DLL、plugins.json、変換済み Resources の配布物生成。
- `verify-runtime`: Plugin JSON、必須 Plugin / 依存 DLL、単一配置、リソース変換の検証。Editor も DLL は検証。
- `generate-plugins --project-dir`: CMake 用構成別選択の生成。

Stage / Package / Verify の依存 DLL は反復可能な `--plugin-dependency`、MinGW runtime は `--mingw-runtime` で指定します。
コマンドごとの必須引数は `broccoli.bat <command> --help` を参照してください。

## テスト

```powershell
uv run --project Tools/Build --frozen pytest Tools/Build/tests
powershell -File Tests/Network/Run-NetworkTests.ps1
```

C++ テストは事前に Debug を build してください。EOS login は `-EOSLogin` で1プロセスだけ実行します。
