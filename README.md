# BROCCOLI ENGINE

BroccoliEngine は、C++20 / raylib / vcpkg をベースとした2Dゲームエンジンおよび開発ツール群です。
エディタ機能、自動化サーバー、ネットワーク同期、パッケージングツールが統合されています。

ゲーム開発時はサブモジュールとして使用されます。

---

## 開発要件

* **OS:** Windows 11 / 10 (x64)
* **C++ コンパイラ:** MSVC (Visual Studio 2026 推奨, C++20 対応)
* **ビルドツール:** CMake 4.2 以上
* **パッケージマネージャー:** [vcpkg](https://github.com/microsoft/vcpkg)
* **Python 環境:** Python `>=3.11, <3.15` および [uv](https://github.com/astral-sh/uv) (ビルドツール・MCP Bridge 用)

---

## 初期セットアップ

### 1. vcpkg のセットアップ

vcpkg をクローンしてセットアップします。

```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### 2. Python ツール環境の同期 (uv)

ビルド・配布パッケージ用ツールの依存関係を同期します。

```cmd
cd Tools/Build
uv sync
```

---

## エンジン開発用 Launcher

エンジンリポジトリ単体でも、同梱の `Launcher` をゲームと同じ CMake 構成でビルド・実行できます。

最初にローカル vcpkg のパスを設定します。

```cmd
copy CMakeUserPresets.json.template CMakeUserPresets.json
```

`CMakeUserPresets.json` の `{YOUR_VCPKG_ROOT_DIRECTORY}` を vcpkg ルートへ置き換えた後、
リポジトリルートで実行してください。

```cmd
build.bat Debug
build.bat Editor
run.bat Debug -automation
```

`Launcher` は `Broccoli::Engine` をリンクし、ゲームプロジェクトと同じリソース変換、
ランタイム配置、およびパッケージング処理を使用します。`-automation` を付けると
Automation HTTP API が `http://127.0.0.1:39100` で有効になります。

---

## ゲームプロジェクトの生成

初期ゲームにはBasicGameplay、PlayerController、レベルJSONが展開されます。

プロジェクト名はASCII英字で始まり、ASCII英数字またはアンダースコアのみ使用でき、`BroccoliEngine` は予約名です。

### テンプレートだけを展開する

Git構成を変更せず、指定先へワークスペース設定と `<ProjectName>/` を生成します。

```cmd
python SetupProject.py --name MyGame --output C:\Projects\MyGame
```

CMake configure前に、生成先の `BroccoliEngine/` へこのエンジンを配置してください。

### 現在のcloneをゲームプロジェクトへ再構成する

クリーンなエンジンcloneのルートで実行します。

```cmd
python SetupProject.py --name MyGame --in-place
```

実行前に、originとHEADが存在すること、未コミット変更がないこと、
`BroccoliEngine/` と `<ProjectName>/` が存在しないことを検証します。
成功後は次の構成になり、エンジンは初期化済みsubmoduleとして登録されます。

```text
MyGame/
├── .git/
├── .gitmodules
├── .gitignore
├── .broccoli-project.json
├── CMakeLists.txt
├── CMakePresets.json
├── build.bat
├── run.bat
├── BroccoliEngine/
└── MyGame/
    ├── CMakeLists.txt
    ├── Source/
    └── Resources/
```

失敗した場合は、新規ルートGit、生成ファイル、移動したエンジン資産を自動的に元へ戻します。

### ゲームプロジェクトからエンジンリポジトリへ戻す

再構成済みワークスペースのルートで次を実行します。

```cmd
python BroccoliEngine\SetupProject.py --reset-project
```

処理は次の順序で行います。

1. `.broccoli-project.json` とgitlinkを検証
2. `BroccoliEngine/.git` に吸収済みGitディレクトリを復元
3. ゲームWorkspaceの全資産を兄弟バックアップへ退避
4. `BroccoliEngine/` の全内容をリポジトリルートへ戻す

バックアップ先は既定で `<Workspace>.broccoli-project-backup` です。
`--backup-output <Path>` で変更できます。復元後のルートは元のエンジンリポジトリ構成となり、
ゲームコードや外側Gitはバックアップから回収できます。


### 生成したゲームのビルドと実行

```cmd
build.bat Debug
build.bat Editor
run.bat Debug
run.bat Editor
```

Editorでは `/Game/*` を `<ProjectName>/Resources/*` へ解決し、配布版では
`Publish/<Config>/Resources/<ProjectName>/*` を使用します。

### セットアップ機能のテスト

高速なテンプレート・gitlink・ロールバックテスト:

```cmd
uv run --project Tools\Build --frozen pytest Tools\Build\tests Tools\ProjectSetup\tests\test_setup_project.py -q -o "python_functions=Test*"
```

クリーンなコミット済みエンジンcloneからの統合テスト:

```powershell
.\Tools\ProjectSetup\tests\Invoke-ProjectSetupIntegration.ps1 -ToolchainFile C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

統合テストは一時ディレクトリで再構成、Debug/Editorビルド、起動、リソース読込、
gitlinkとsubmodule状態を検証し、成功後に一時成果物を削除します。
