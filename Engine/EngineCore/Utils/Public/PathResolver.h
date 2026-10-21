#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API PathResolver {
 public:
  static std::string Resolve(const std::string& Path);
  static std::string SanitizeResourcePath(const std::string& Path);

  /// @brief windowsパスからエンジンで使用する仮想パスを取得する。
  /// @param Path
  /// @return
  static std::optional<std::string> MakeVirtualPath(const std::string& Path);

  /// @brief エンジン側リソースディレクトリのパスを取得。
  /// @return
  static std::string GetEngineResourceDir();

  /// @brief ゲーム側リソースディレクトリのパスを取得。
  /// @return ゲームフォルダ/Resources/のパス
  static std::string GetGameResourceDir();
  static std::filesystem::path GetExecutableDirectory();
  static void SetProjectRoot(const std::string& Root);
  static const std::string& GetProjectRoot();
  static void SetGameName(const std::string& Name);
  static const std::string& GetGameName();
  static void InitializeWorkingDirectory();
};