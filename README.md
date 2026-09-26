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
* **Python 環境:** Python `>=3.11, <3.15` および [uv](https://github.com/astral-sh/uv)

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

`CMakeUserPresets.json` の `{YOUR_VCPKG_ROOT_DIRECTORY}` を vcpkg ルートへ置き換えてください。

```cmd
broccoli.bat build
broccoli.bat run --latest
```

---

## エンジン開発用 Launcher

エンジンリポジトリ単体でも、同梱の `Launcher` をゲームと同じ CMake 構成でビルド・実行できます。

`CMakeUserPresets.json` の `{YOUR_VCPKG_ROOT_DIRECTORY}` を vcpkg ルートへ置き換えた後、
リポジトリルートで実行してください。

```cmd
broccoli.bat build Debug
broccoli.bat run Debug
```

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
