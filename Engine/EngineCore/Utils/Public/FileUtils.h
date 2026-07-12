#pragma once
#include "BroccoliEngineAPI.h"

#include <filesystem>
#include <string>

class BROCCOLI_ENGINE_API FileUtils {
 public:
  static bool IsPathInsideProject(const std::filesystem::path& path);

  static std::string GetProjectRelativePath(const std::string& fullPath);

 private:
  struct Impl;
  static Impl* GetImpl();
};
