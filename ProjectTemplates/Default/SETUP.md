## 生成済みゲームプロジェクトをクローンした後のセットアップ

`SetupProject.py --in-place` などで生成済みのゲームプロジェクトを別環境へクローンした場合は、以下の手順で開発環境をセットアップします。

### 1. リポジトリとsubmoduleの取得

```cmd
git clone --recurse-submodules <Repository URL>
cd <ProjectName>
```

すでに通常の `git clone` を実行済みの場合は、submoduleを初期化します。

```cmd
git submodule update --init --recursive
```

### 2. vcpkg のセットアップ

vcpkg が未導入の場合は、任意の場所へクローンしてセットアップします。

```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### 3. uv のインストール

`uv` が未導入の場合は、wingetからインストールできます。

```cmd
winget install astral-sh.uv
```

### 4. BroccoliEngine のPythonツール環境を同期

ゲームプロジェクトのルートから実行します。

```cmd
cd BroccoliEngine\Tools\Build
uv sync
cd ..\..\..
```

### 5. CMake User Preset の設定

ローカル環境固有のvcpkgパスは`CMakeUserPresets.json` に設定します。

プロジェクトルートへ `CMakeUserPresets.json` を作成します。

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-x64-local",
      "inherits": "windows-x64",
      "environment": {
        "VCPKG_ROOT": "C:/vcpkg"
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

`C:\vcpkg` 以外へvcpkgを配置している場合は、自分の環境に合わせて変更してください。

`CMakeUserPresets.json` は開発環境固有の設定なので、Git管理対象には含まれません。

### 6. CMake configure

CMakeを直接使用する場合は、設定したpresetでconfigureします。

```cmd
cmake --preset windows-x64-local
```

### 7. ビルド

CMakeから直接ビルドする場合:

```cmd
cmake --build --preset debug-local
```

または、プロジェクトルートの統合CLIを使用します。

```cmd
broccoli.bat build Debug
```

Editorビルドの場合:

```cmd
broccoli.bat build Editor
```

ビルド後は次のコマンドで起動できます。

```cmd
broccoli.bat run Debug
broccoli.bat run Editor
```

broccoli.bat --helpで詳細を確認してください。
