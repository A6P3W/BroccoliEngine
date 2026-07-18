変数名は大文字スタート
m_は不要
.h,.cppファイルはUTF-8 with BOMで保存すること。
行の終わりはCRLF。

変更したファイルには.clang-formatを適用すること。

ビルドにはmsbuildを使用。

/t:Build /p:Configuration=Debug /p:Platform=x64 /m

editor関連のデバッグ中は Configuration=Editorを使用。

BroccoliEngineソリューションの変更後のビルドテスト方法

1. BroccoliEngine.vcxprojのビルド、
2. 成功したらBroccoliEngine.slnxのビルド。


エンジンを使用したゲーム開発時
ゲームソリューションの変更後のビルドテスト方法
1. Engineコードが変更された初回のみBroccoliEngine.vcxprojのビルド。pull後など。
2. ゲーム.slnxのビルド。

起動方法：Start-Process -FilePath '{SolutionPath}\Publish\Debug\Launcher.exe
