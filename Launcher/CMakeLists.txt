# Launcher (WIN32 Application)

add_executable(Launcher WIN32 Entry.cpp)

# 依存プロジェクトのリンク
target_link_libraries(Launcher PRIVATE BroccoliEngine)

# 静的ライブラリ内のグローバルオブジェクト強制初期化のための WHOLEARCHIVE 指定
if(MSVC)
  target_link_options(Launcher PRIVATE "/WHOLEARCHIVE:BroccoliEngine")
endif()

# インクルードディレクトリの設定
target_include_directories(Launcher PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)



# MSVC 固有のコンパイラ警告/動作設定
if(MSVC)
  target_compile_options(Launcher PRIVATE 
    "/MP"       # マルチプロセッサビルド
    "/utf-8"    # ソースコードの文字コードをUTF-8に強制
    "/wd4819"   # 特定の警告コードの抑制
  )

  # Visual Studio 上でのデバッグ時の作業ディレクトリを設定
  set_target_properties(Launcher PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VS_CHAR_SET "MultiByte"
  )
endif()


