# ビルド環境セットアップ手順 (BuildSetup)

BroccoliEngine をビルドできるようにするためのプロジェクト初期セットアップ手順です。

※ 開発ツール自体のインストールがまだお済みでない場合は、先に [ToolSetup.md](./ToolSetup.md) を参照してインストールを完了してください。

---

## 1. Python ツール環境の同期 (uv)

BroccoliEngine では、ビルド成果物のランタイム配置、パッケージング、レベル変換などの処理を内製 Python ツールで行います。そのため、ツールの依存関係を事前に同期しておく必要があります。

リポジトリルートから以下を実行します：

```cmd
cd Tools\Build
uv sync
cd ..\..
```

> **Note (ゲームプロジェクトの場合):**  
> 展開済みゲームプロジェクトの場合は、`cd BroccoliEngine\Tools\Build` で実行してください。

---

## 2. CMake User Preset の設定 (CMakeUserPresets.json)

ローカル環境固有の vcpkg パスおよび MinGW-w64 GCC のパスを設定します。

リポジトリルートでテンプレートファイルをコピーして `CMakeUserPresets.json` を作成します：

```cmd
copy CMakeUserPresets.json.template CMakeUserPresets.json
```

作成された `CMakeUserPresets.json` をテキストエディタで開き、以下のプレースホルダーをご自身の環境に合わせて置き換えます：

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-x64-local",
      "displayName": "Windows x64 Local",
      "description": "Ninja Multi-Config / MinGW-w64 GCC / local vcpkg override",
      "inherits": "windows-x64",
      "environment": {
        "VCPKG_ROOT": "{YOUR_VCPKG_ROOT_DIRECTORY}",
        "PATH": "{YOUR_MINGW_BIN_DIRECTORY};$penv{PATH}"
      },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "{YOUR_MINGW_BIN_DIRECTORY}/gcc.exe",
        "CMAKE_CXX_COMPILER": "{YOUR_MINGW_BIN_DIRECTORY}/g++.exe"
      }
    }
  ],
  ...
}
```

### 入力項目の説明

* **`{YOUR_VCPKG_ROOT_DIRECTORY}`**:  
  vcpkg のルートディレクトリパスを指定します。（例: `C:/vcpkg`）
* **`{YOUR_MINGW_BIN_DIRECTORY}`**:  
  MinGW-w64 GCC および Ninja が配置されている `bin` ディレクトリパスを指定します。（例: `C:/msys64/mingw64/bin`）

> **Note:** `CMakeUserPresets.json` は個人環境設定のため、Git 管理対象外となっています。

---

## 3. 初期 Configure の確認（任意）

設定が正しいか確認したい場合は、リポジトリルートで CMake の configure を直接実行して確認できます：

```cmd
cmake --preset windows-x64-local
```

エラーが表示されず正常に完了すれば、ビルド環境の準備は完了です。

---
