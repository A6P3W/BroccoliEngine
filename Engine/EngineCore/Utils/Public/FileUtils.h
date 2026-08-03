#pragma once
#include "BroccoliEngineAPI.h"

#include <filesystem>
#include <string>

class BROCCOLI_ENGINE_API FileUtils {
 public:
  static std::filesystem::path Utf8ToPath(const std::string& utf8Str) {
    if (utf8Str.empty()) return std::filesystem::path();
    return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8Str.c_str()));
  }

  static std::string PathToUtf8(const std::filesystem::path& path) {
    const auto u8Str = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8Str.c_str()), u8Str.length());
  }

  static bool IsPathInsideProject(const std::filesystem::path& path);

  static std::string GetProjectRelativePath(const std::string& fullPath);

 private:
  struct Impl;
  static Impl* GetImpl();
};
