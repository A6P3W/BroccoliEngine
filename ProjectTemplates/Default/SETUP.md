# ゲームプロジェクト セットアップ手順

生成済みのゲームプロジェクトをクローンした後の開発環境セットアップ手順です。

---

## 1. リポジトリと submodule の取得

```cmd
git clone --recurse-submodules <Repository URL>
cd <ProjectName>
```

すでに通常の `git clone` を実行済みの場合は、submodule を初期化します：

```cmd
git submodule update --init --recursive
```

---

## 2. 開発ツールのインストール（未導入の場合）

MinGW-w64 GCC、CMake、vcpkg、Python、uv などのツールのインストールがまだお済みでない場合は、以下のドキュメントを参照して導入してください。

* **[開発ツールセットアップ手順 (ToolSetup)](BroccoliEngine/Engine/Documents/ToolSetup.md)**

---

## 3. BroccoliEngine の Python ツール環境を同期

ゲームプロジェクトのルートから実行します。

```cmd
cd BroccoliEngine\Tools\Build
uv sync
cd ..\..\..
```

---

## 4. CMake User Preset の設定

ローカル環境固有の vcpkg と MinGW-w64 GCC のパスは `CMakeUserPresets.json` に設定します。

プロジェクトルートへ `CMakeUserPresets.json` を作成します。

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-x64-local",
      "inherits": "windows-x64",
      "environment": {
        "VCPKG_ROOT": "C:/vcpkg",
        "PATH": "C:/msys64/mingw64/bin;$penv{PATH}"
      },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "C:/msys64/mingw64/bin/gcc.exe",
        "CMAKE_CXX_COMPILER": "C:/msys64/mingw64/bin/g++.exe"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "debug-local",
      "inherits": "debug",
      "configurePreset": "windows-x64-local"
    },
    {
      "name": "editor-local",
      "inherits": "editor",
      "configurePreset": "windows-x64-local"
    },
    {
      "name": "release-local",
      "inherits": "release",
      "configurePreset": "windows-x64-local"
    }
  ]
}
```

ご自身の環境に合わせて変更してください。

> **Note:** `CMakeUserPresets.json` は個人環境設定のため、Git 管理対象外となっています。

---

## 5. ビルドと実行

### 統合 CLI を使用する場合

```cmd
# Debug ビルドと起動
broccoli.bat build Debug
broccoli.bat run Debug

# Editor ビルドと起動
broccoli.bat build Editor
broccoli.bat run Editor
```

詳細は `broccoli.bat --help` を参照してください。
