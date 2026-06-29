#pragma once
#include <filesystem>
#include <string>

class FileUtils {
 public:
  static bool IsPathInsideProject(const std::filesystem::path& path);

  static std::string GetProjectRelativePath(const std::string& fullPath);
};
