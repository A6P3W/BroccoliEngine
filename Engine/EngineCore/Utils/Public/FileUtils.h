#pragma once
#include "BroccoliEngineAPI.h"

#include <filesystem>
#include <string>

class BROCCOLI_ENGINE_API FileUtils {
 public:
  static std::filesystem::path Utf8ToPath(const std::string& Utf8Str) {
    if (Utf8Str.empty()) return std::filesystem::path();
    return std::filesystem::path(reinterpret_cast<const char8_t*>(Utf8Str.c_str()));
  }

  static std::string PathToUtf8(const std::filesystem::path& Path) {
    const auto U8Str = Path.u8string();
    return std::string(reinterpret_cast<const char*>(U8Str.c_str()), U8Str.length());
  }

  static std::string PathToUtf8Generic(const std::filesystem::path& Path) {
    const auto U8Str = Path.generic_u8string();
    return std::string(reinterpret_cast<const char*>(U8Str.c_str()), U8Str.length());
  }

  static bool IsPathInsideProject(const std::filesystem::path& path);

  static std::string GetProjectRelativePath(const std::string& fullPath);

 private:
  struct Impl;
  static Impl* GetImpl();
};
