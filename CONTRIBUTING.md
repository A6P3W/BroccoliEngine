# Contributing Guide

このリポジトリへのコントリビューション手順を記載します。

## 開発フロー

1. `main` ブランチから作業ブランチを作成
2. 実装・修正
3. テストを実施
4. コミット (1つの目的を持ち、そのコミット単体で説明・レビュー・取り消しができる変更の単位)
5. Draft Pull Requestを作成
6. 機能単位ごとに2,3, 4を繰り返す
7. PRの状態を `Ready for review` に変更
8. レビュー・承認後にマージ

## ブランチ命名規則

以下の形式を使用してください。

```text
<type>/<description>
```

例:

```text
feature/add-login
```

主な `type`:

* `feature`: 機能追加
* `fix`: バグ修正
* `refactor`: リファクタリング
* `docs`: ドキュメント変更

## コミットメッセージ

Conventional Commitsをベースに、以下の形式を使用します。

```text
<type>(<scope>): <summary(日本語)>
```

例:

```text
feat(auth): ログイン機能を追加
```

主な `type`:

* `feat`: 機能追加
* `fix`: バグ修正
* `docs`: ドキュメント変更
* `refactor`: リファクタリング

## Pull Request

.github/pull_request_template.md を参照してください。

GithubActions以外で行ったテストについては、.github/pr_test_comment_template.md を参照し、テスト結果をPull Requestにコメントしてください。

## マージ

マージ方法は `Squash and merge` を基本とします。
