# 開発ツールセットアップ手順

BroccoliEngine および BroccoliEngine を使用したゲーム開発に必要な各種ツールのインストール手順です。

---

## 開発要件

* **OS:** Windows 11 / 10 (x64)
* **C++ コンパイラ:** MinGW-w64 GCC 16.2.0 以上（C++26 静的リフレクション `-freflection` 必須）
* **ビルドツール:** CMake 4.2 以上、Ninja
* **パッケージマネージャー:** [vcpkg](https://github.com/microsoft/vcpkg)
* **Python 環境:** Python `>=3.11, <3.15` および [uv](https://github.com/astral-sh/uv)
* **デバッガ（任意）:** MSYS2 GDB (`mingw-w64-x86_64-gdb`)
* **VS Code 拡張（任意）:** `C/C++` (`ms-vscode.cpptools`), `CMake Tools` (`ms-vscode.cmake-tools`)

---

## 各ツールのインストール

### 1. MSYS2（GCC・GDB・Ninja）のインストール

GCC コンパイラ、GDB デバッガ、Ninja は MSYS2 を経由してインストールします。

#### ① MSYS2 本体のインストール

PowerShell またはコマンドプロンプトで以下を実行します：

```powershell
winget install MSYS2.MSYS2
```

（または [MSYS2 公式サイト](https://www.msys2.org/) からインストーラをダウンロードして実行）

#### ② GCC / GDB / Ninja のインストール

Windows のスタートメニューから **「MSYS2 MINGW64」** ターミナルを起動し、以下のコマンドを実行します：

```bash
# パッケージデータベースの更新
pacman -Syu

# GCC、GDB、Ninja のインストール
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-ninja
```

* **インストール先:**  
  `C:\msys64\mingw64\bin` 配下に `gcc.exe`, `g++.exe`, `gdb.exe`, `ninja.exe` が配置されます。

---

### 2. CMake 4.2 以上のインストール

PowerShell またはコマンドプロンプトで以下を実行します：

```powershell
winget install Kitware.CMake
```

（または [CMake 公式ダウンロードページ](https://cmake.org/download/) から Windows x64 Installer を入手）

※ インストール時に「Add CMake to the system PATH for all users (または current user)」にチェックを入れてください。

---

### 3. パッケージマネージャー: vcpkg

任意のディレクトリへクローンしてセットアップします（以下は例として `C:\vcpkg` に配置する場合の手順です。別の場所に配置した場合は、後の `CMakeUserPresets.json` 設定でそのパスを指定してください）。

コマンドプロンプト（または PowerShell）で以下を実行します：

```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

---

### 4. Python（3.11～3.14）および uv

#### ① Python

PowerShell で以下を実行します：

```powershell
winget install Python.Python.3.12
```

#### ② uv（高速 Python パッケージマネージャー）

PowerShell で以下を実行します：

```powershell
winget install astral-sh.uv
```

---

---

## インストールの確認

PowerShell を新しく開き、以下を実行して各ツールが認識されているか確認できます：

```powershell
# MinGW ツール群
C:\msys64\mingw64\bin\g++.exe --version
C:\msys64\mingw64\bin\gdb.exe --version
C:\msys64\mingw64\bin\ninja.exe --version

# ビルドツール & Python
cmake --version
python --version
uv --version
```
