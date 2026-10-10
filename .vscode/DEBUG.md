# VS Code での GDB デバッグ手順

VS Code と GDB を使用して、BroccoliEngine（Launcher）のソースレベルデバッグを行う手順です。

---

## 前提条件

* **VS Code 拡張機能**: `C/C++` (`ms-vscode.cpptools`) および `CMake Tools` (`ms-vscode.cmake-tools`) がインストールされていること（`.vscode/extensions.json` 推奨）
* **デバッガ**: MSYS2 GDB (`C:/msys64/mingw64/bin/gdb.exe`) がインストールされていること（未導入の場合は [ToolSetup.md](../Engine/Documents/ToolSetup.md) 参照）
* **初期セットアップ**: `CMakeUserPresets.json` の設定が完了していること（未設定の場合は [BuildSetup.md](../Engine/Documents/BuildSetup.md) 参照）

---

## デバッグ起動手順 (F5)

1. **プリセットの選択**  
   VS Code のステータスバー（または CMake Tools パネル）から、以下を選択します：
   * **Configure Preset:** `windows-x64-local`
   * **Build Preset:** `debug-local`（または `editor-local`）

2. **デバッグの開始**  
   `F5` キーを押すか、VS Code 左側の「実行とデバッグ」パネルから **「GDB: Launcher (active CMake preset)」** を選択して実行します。  
   CMake Tools により対象ターゲット（`Launcher`）が自動で差分ビルドされ、GDB 経由でデバッグ起動します。

3. **ブレークポイントとステップ実行**  
   ソースコードの行番号の左側をクリックしてブレークポイントを設置できます。
   * **Launcher / ゲームコード:** `Launcher/Source/Entry.cpp`（`WinMain`, `SetupGame` など）
   * **エンジン DLL:** `Engine/EngineCore/Core/Private/Application.cpp`（`Application::Run` など）
   * **プラグイン DLL:** `Engine/Plugins/ExamplePlugin/Source/Private/ExamplePlugin.cpp` など
   * `F10`（ステップオーバー）、`F11`（ステップイン）、ローカル変数の参照、コールスタック（呼び出し履歴）の確認が可能です。

---

## 設定仕様 (`launch.json`)

* **`program`**: CMake Tools の `cmake.launchTargetPath`（`targetName: Launcher`）により、選択中のプリセットに応じた exe パスを自動解決します。
* **`cwd`**: `cmake.getLaunchTargetDirectory` により exe の出力ディレクトリが設定され、ステージング済み Resources や DLL の相対パスが正しく解決されます。
* **`miDebuggerPath`**: `C:/msys64/mingw64/bin/gdb.exe` を指定しています。環境により配置先が異なる場合はこのパスを変更してください。
