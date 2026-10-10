# BROCCOLI ENGINE

BroccoliEngine は、C++26 / raylib / vcpkg をベースとした2Dゲームエンジンおよび開発ツール群です。
エディタ機能、自動化サーバー、ネットワーク同期、パッケージングツールが統合されています。

ゲーム開発時はサブモジュールとして使用されます。

---

## 開発要件

* **OS:** Windows 11 / 10 (x64)
* **C++ コンパイラ:** MinGW-w64 GCC 16.2.0 以上
* **ビルドツール:** CMake 4.2 以上、Ninja
* **パッケージマネージャー:** [vcpkg](https://github.com/microsoft/vcpkg)
* **Python 環境:** Python `>=3.11, <3.15` および [uv](https://github.com/astral-sh/uv)
* **デバッガ（任意）:** MSYS2 GDB (`mingw-w64-x86_64-gdb`)
* **VS Code 拡張（任意）:** `C/C++` (`ms-vscode.cpptools`), `CMake Tools` (`ms-vscode.cmake-tools`)

---

## 初期セットアップ

BroccoliEngine 本体の開発環境をセットアップする手順です。

1. **ステップ 1: 開発ツールのインストール（未導入の場合）**  
[ToolSetup.md](Engine/Documents/ToolSetup.md) を参照してインストールを行ってください。
2. **ステップ 2: ビルド環境の準備（初期セットアップ）**  
[BuildSetup.md](Engine/Documents/BuildSetup.md) を参照して初期セットアップを行ってください。

---

## ゲームプロジェクトの生成

初期ゲームにはサンプルレベルが展開されます。

プロジェクト名はASCII英字で始まり、ASCII英数字またはアンダースコアのみ使用できます。

### 現在のcloneをゲームプロジェクトへ再構成する

クリーンなエンジンのルートで実行します。

```cmd
python SetupProject.py --name MyGame --in-place
```

成功後は次の構成になり、エンジンはsubmoduleとして登録されます。

```text
MyGame/
├── BroccoliEngine/
└── MyGame/
    ├── Source/
    └── Resources/
```

### 生成したゲームのビルドと実行

ローカル vcpkg のパスを設定します。

```cmd
copy CMakeUserPresets.json.template CMakeUserPresets.json
```

`CMakeUserPresets.json` の `{YOUR_VCPKG_ROOT_DIRECTORY}` を vcpkg ルートへ、
`{YOUR_MINGW_BIN_DIRECTORY}` を GCC と Ninja が入った `bin` ディレクトリへ置き換えてください。

```cmd
broccoli.bat build
broccoli.bat run --latest
```

---

## エンジン開発用 Launcher

初期セットアップ完了後、エンジンリポジトリ単体でも同梱の `Launcher` をビルド・実行できます。

### ビルドと実行

```cmd
broccoli.bat build Debug
broccoli.bat run Debug
```

### VS Code でのデバッグ (F5)

VS Code と GDB を使用したソースレベルデバッグの手順については、[.vscode/DEBUG.md](.vscode/DEBUG.md) を参照してください。

### Control CLI

`--control` を指定すると、`broccoli.bat control` から Engine を操作できます。

[Auto-Control-C++使用方法](./Engine/Documents/Auto-Control-C++使用方法.md)

[Auto-Control-CLI使用方法](./Engine/Documents/Auto-Control-CLI使用方法.md)

---

## worktree のセットアップ

Git 標準コマンドで worktree を作成した後、Broccoli 固有のローカル開発環境を引き継ぎます。

```cmd
git worktree add -b feature/example ../BroccoliEngine-worktrees/feature-example HEAD
broccoli.bat worktree setup ../BroccoliEngine-worktrees/feature-example
```

存在する場合は `CMakeUserPresets.json` を新しい worktree へコピーします。
