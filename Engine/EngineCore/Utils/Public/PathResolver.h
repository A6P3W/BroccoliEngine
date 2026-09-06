#pragma once

#include <filesystem>
#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API PathResolver {
 public:
  static std::string Resolve(const std::string& Path);
  static std::string SanitizeResourcePath(const std::string& Path);
  static std::string GetEngineResourceDir();
  static std::string GetGameResourceDir();
  static std::filesystem::path GetExecutableDirectory();
  static void SetProjectRoot(const std::string& Root);
  static const std::string& GetProjectRoot();
  static void SetGameName(const std::string& Name);
  static const std::string& GetGameName();
  static void InitializeWorkingDirectory();
};
