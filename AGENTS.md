変数名は大文字スタート
m\_は不要
.h,.cppファイルはUTF-8 with BOMで保存すること。
行の終わりはCRLF。
アプリケーションログはLogsフォルダに出力される。



ビルドにはmsbuildを使用。

/t:Build /p:Configuration=Debug /p:Platform=x64 /m

BroccoliEngineプロジェクトの変更後のビルドテスト方法

BroccoliEngine.vcxprojのビルド、成功したらBroccoliEngine.slnxのビルド。

