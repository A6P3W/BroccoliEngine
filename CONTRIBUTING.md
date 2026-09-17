# Contributing Guide

このリポジトリへのコントリビューション手順を記載します。

## 開発フロー

1. `main` ブランチから作業ブランチを作成
2. 実装・修正
3. テストを実施
4. コミット
5. Pull Requestを作成
6. レビュー・承認後にマージ

## ブランチ命名規則

以下の形式を使用してください。

```text
<type>/<description>
```

例:

```text
feature/add-login
fix/user-api-error
refactor/auth-service
docs/update-readme
```

主な `type`:

* `feature`: 機能追加
* `fix`: バグ修正
* `refactor`: リファクタリング
* `docs`: ドキュメント変更
* `test`: テスト関連
* `chore`: 設定・依存関係など

## コミットメッセージ

Conventional Commitsをベースに、以下の形式を使用します。

```text
<type>(<scope>): <summary>
```

例:

```text
feat(auth): ログイン機能を追加
fix(user): ユーザー取得時のエラーを修正
refactor(api): 認証処理を共通化
docs(readme): セットアップ手順を更新
```

主な `type`:

* `feat`: 機能追加
* `fix`: バグ修正
* `docs`: ドキュメント変更
* `refactor`: リファクタリング
* `test`: テスト追加・修正
* `chore`: 設定・依存関係など
* `ci`: CI/CD関連

コミットは、可能な限り1つの目的につき1コミットにしてください。

## Pull Request

PRには以下を記載してください。

* 変更概要
* 主な変更内容
* 変更理由
* 動作確認内容
* 関連Issue

## レビュー

PR作成者は、レビュー依頼前に以下を確認してください。

* Lint / Formatエラーがない
* 不要なデバッグコードがない
* 変更範囲に不要なファイルが含まれていない
* PRの説明だけで変更目的が理解できる

レビューで修正依頼があった場合は、原則として同じPR内で対応してください。

## マージ

原則として、以下の条件を満たした後にマージします。

* 必要なレビューが完了している
* 未解決のレビューコメントがない

マージ方法は `Squash and merge` を基本とします。
