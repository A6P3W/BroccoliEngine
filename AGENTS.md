変数名は大文字スタート
m_は不要
charset = utf-8
end_of_line = lf

変更したファイルには.clang-formatを適用し、ファイル保存形式も従う。

BroccoliEngineソリューションの変更後のビルドテスト方法
BroccoliEngineフォルダの./broccoli.bat build {必要な構成}

必要な構成はDebug必須、Editorが必要とされる場合はEditorも指定すること。

c++20環境

開発開始前に確認：CONTRIBUTING.md


control機能使用方法
[Auto-Control-CLI](Engine\Documents\Auto-Control-CLI使用方法.md)

broccoli.batを使用してビルド、実行。
詳細はbroccoli.bat --helpを参照。
