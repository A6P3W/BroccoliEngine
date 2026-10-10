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

## 3. ビルド環境の準備（初期セットアップ）

ツールの導入完了後、Python ツール環境の同期および `CMakeUserPresets.json` の設定が必要です。詳細な手順は以下のドキュメントを参照してください。

* **[ビルド環境セットアップ手順 (BuildSetup)](BroccoliEngine/Engine/Documents/BuildSetup.md)**

---

## 4. ビルドと実行

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

---

## 5. VS Code でのデバッグ (F5)

VS Code と GDB を使用したソースレベルデバッグの手順については、[.vscode/DEBUG.md](.vscode/DEBUG.md) を参照してください。
