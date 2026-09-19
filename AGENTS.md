変数名は大文字スタート
m_は不要
charset = utf-8
end_of_line = lf

変更したファイルには.clang-formatを適用すること。

BroccoliEngineソリューションの変更後のビルドテスト方法
BroccoliEngineフォルダの./broccoli.bat build {必要な構成}

ゲームソリューションの変更後のビルドテスト方法
ゲームフォルダの./broccoli.bat build {必要な構成}

c++20環境

運用はCONTRIBUTING.mdに従ってもらいますが、テストにおいて画面操作など開発者が行うべきと判断した場合にはテスト項目を提示すること。