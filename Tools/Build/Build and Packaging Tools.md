# BroccoliEngine Build and Packaging Tools

BroccoliEngine のビルド成果物のクリーンアップ、ローカル実行用のステージング、配布用パッケージの作成、および成果物の検証を行うための Python スクリプトツール群です。

## 概要

このプロジェクトは `broccoli_build` パッケージを含む Python ツールで、主に CMake ビルドプロセスなどから呼び出され、ビルド成果物を処理するために使用されます。

## 動作要件

- Python `>=3.11, <3.15`
- パッケージ管理/仮想環境構築ツールとして `uv` が推奨されています（`uv.lock` が提供されています）。

## セットアップ手順

プロジェクトディレクトリ（`Tools/Build`）で依存関係をセットアップするには、`uv` を使用して以下を実行します。

```bash
# 依存関係のインストールと仮想環境の構築
uv sync
```

## 使用方法

メインのエントリポイントは `broccoli_build` モジュールです。

```bash
uv run python -m broccoli_build <コマンド> [引数...]
```

### 利用可能なコマンド

#### 1. `prepare-output`
ビルド出力ディレクトリから、前回の実行などで生成された一時ファイル（ログや保存データ、一時リソースなど）を削除し、クリーンな状態にします。

- **引数:**
  - `--output-dir`: 出力先ディレクトリのパス（必須）

```bash
uv run python -m broccoli_build prepare-output --output-dir <パス>
```

#### 2. `stage-runtime`
ローカル実行に必要なエンジンリソースやゲームリソース、バイナリなどを出力ディレクトリにコピーし、レベルのコンバート（`ConvertLevels.py` の実行）を行います。

- **引数:**
  - `--configuration`: ビルド構成（`Debug`、`Release`、`Editor` など）
  - `--engine-dir`: エンジンソースのルートディレクトリ（必須）
  - `--game-dir`: ゲームソースのルートディレクトリ（必須）
  - `--output-dir`: 出力先ディレクトリ（必須）
  - `--engine-binary`: エンジンバイナリ（`BroccoliEngine.dll` など）のパス（必須）
  - `--game-name`: ゲーム名（必須）
  - `--eos-binary`: EOSバイナリのパス（必須）
  - `--convert-levels-script`: レベル変換スクリプト（`ConvertLevels.py` など）のパス（必須）

#### 3. `package-runtime`
配布可能なランタイムパッケージ（ゲーム実行に必要なバイナリやリソースがまとめられた構成）をパブリッシュディレクトリに構築します。

- **引数:**
  - `--configuration`: ビルド構成（必須）
  - `--output-dir`: 出力ディレクトリ（必須）
  - `--publish-dir`: パブリッシュ（配布物生成）先ディレクトリ（必須）
  - `--game-binary`: ゲームバイナリのパス（必須）
  - `--engine-binary`: エンジンバイナリのパス（必須）
  - `--game-name`: ゲーム名（必須）
  - `--eos-binary`: EOSバイナリのパス（必須）
  - `--online-resources-dir`: EOSオンラインリソースディレクトリのパス（必須）
  - `--convert-levels-script`: レベル変換スクリプトのパス（必須）
  - `--bootstrap-binary`: ブートストラップバイナリ（`BroccoliBootstrap.exe` など）のパス（必須）

#### 4. `verify-runtime`
ステージングされた、あるいはパッケージ化された成果物が必要な構成を満たしているか（必要な DLL やリソースがあるか、未変換のレベルファイル `.BLevel.json` が残っていないか）を検証します。

- **引数:**
  - `--output-dir`: 出力ディレクトリ（必須）
  - `--game-name`: ゲーム名（必須）
  - `--publish-dir`: パブリッシュディレクトリ（任意）
  - `--configuration`: ビルド構成（任意）

## テストの実行

`pytest` を使用してユニットテストを実行できます。

```bash
uv run pytest
```
