# BROCCOLI ENGINE

BroccoliEngine は、C++20 / raylib / vcpkg をベースとした2Dゲームエンジンおよび開発ツール群です。
エディタ機能、自動化サーバー、ネットワーク同期、パッケージングツールが統合されています。

---

## 開発要件

* **OS:** Windows 11 / 10 (x64)
* **C++ コンパイラ:** MSVC (Visual Studio 2026 推奨, C++20 対応)
* **ビルドツール:** CMake 3.25 以上
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
.\vcpkg integrate install
```

### 2. CMakeUserPresets.json の作成

`CMakeUserPresets.json.bak` を複製し、リポジトリルートに `CMakeUserPresets.json` を作成します。
```cmd
copy CMakeUserPresets.json.bak CMakeUserPresets.json
```
作成後、`CMakeUserPresets.json` 内の `configurePresets` にある `local-windows-x64` プリセットの `cacheVariables` の `CMAKE_TOOLCHAIN_FILE` フィールドに、ローカルの vcpkg ツールチェーンパス（例: `C:/vcpkg/scripts/buildsystems/vcpkg.cmake`）を設定します。


### 3. Python ツール環境の同期 (uv)

ビルド・配布パッケージ用ツールの依存関係を同期します。

```cmd
cd Tools/Build
uv sync
```

---

## ビルドと実行

ルートディレクトリに用意されているスクリプトでビルドと実行を行えます。

### ビルド (`build.bat`)

ビルド構成として `Debug`, `Editor`, `Release` が指定可能です（デフォルト: `Debug`）。

```cmd
rem Debug ビルド
build.bat Debug

rem Editor ビルド (レベルエディタ機能付き)
build.bat Editor

rem Release ビルド (本番リリース用)
build.bat Release
```

### 実行 (`run.bat`)

指定した構成のバイナリを実行します。追加引数も渡せます。

```cmd
rem Debug ビルドの実行
run.bat Debug

rem Editor ビルドの実行
run.bat Editor

rem 自動化サーバーを有効化して実行
run.bat Debug -automation
```
