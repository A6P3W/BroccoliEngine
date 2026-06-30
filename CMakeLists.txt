cmake_minimum_required(VERSION 3.20)

# vcpkg ツールチェーンの自動フォールバックロード
if(NOT CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")
  elseif(EXISTS "C:/UserFiles/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/UserFiles/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")
  endif()
endif()

project(BroccoliEngine LANGUAGES C CXX)

# 単体ビルド（Engineフォルダ直下を直接ビルド）の時のみ、依存パッケージの検索を実行
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg target triplet")
  
  find_package(cpr CONFIG REQUIRED)
  find_package(imgui CONFIG REQUIRED)
  find_package(unofficial-enet CONFIG REQUIRED)
  add_library(enet::enet ALIAS unofficial::enet::enet)
  
  # DxLib ディレクトリのデフォルト設定
  if(NOT DXLIB_DIR)
    set(DXLIB_DIR "C:/UserFiles/DxLib" CACHE PATH "DxLib Directory")
  endif()
endif()

# BroccoliEngine (STATIC Library)


# ソースおよびヘッダーファイルを再帰的に収集
file(GLOB_RECURSE BROCCOLLI_SOURCES
  "Engine/*.cpp"
  "Engine/*.h"
  "Engine/*.hpp"
)



# IDE上の表示フォルダ構造を物理フォルダ階層にマッピングする設定
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Engine" PREFIX "Engine" FILES ${BROCCOLLI_SOURCES})

add_library(BroccoliEngine STATIC ${BROCCOLLI_SOURCES})

# C++規格を C++20 に設定
target_compile_features(BroccoliEngine PUBLIC cxx_std_20)


# インクルードディレクトリの設定
target_include_directories(BroccoliEngine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore"

  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/Core/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/Systems/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/Network/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/Components/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineSide"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineSide/Default/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/GameFramework/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/ThirdParty"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Editor/Public"
  "${CMAKE_CURRENT_SOURCE_DIR}/Engine/EngineCore/Utils/Public"
)

# DxLib のヘッダーからの警告を抑制するため SYSTEM インクルードとして追加
target_include_directories(BroccoliEngine SYSTEM PUBLIC "${DXLIB_DIR}")

# ライブラリディレクトリの設定 (DxLib 用)
target_link_directories(BroccoliEngine PUBLIC "${DXLIB_DIR}")

# 外部ライブラリおよびシステムライブラリのリンク
target_link_libraries(BroccoliEngine PUBLIC
  cpr::cpr
  imgui::imgui
  enet::enet
  ws2_32
  winmm
  crypt32
  bcrypt
  secur32
  iphlpapi
)

# プリプロセッサ定義の設定
target_compile_definitions(BroccoliEngine PUBLIC
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Editor>:_EDITOR>
  $<$<CONFIG:Release>:_RELEASE>
  _CONSOLE
)


# MSVC 固有のコンパイラ警告/動作設定
if(MSVC)
  target_compile_options(BroccoliEngine PRIVATE 
    "/MP"       # マルチプロセッサビルド
    "/utf-8"    # ソースコードの文字コードをUTF-8に強制
    "/wd4819"   # 特定の警告コードの抑制 (改行コード/コードページ関連)
    "/wd4828"   # 文字コードページ警告の抑制
  )
  set_target_properties(BroccoliEngine PROPERTIES
    VS_CHAR_SET "MultiByte"
  )
endif()



# サブプロジェクト Launcher の追加
add_subdirectory(Launcher)
