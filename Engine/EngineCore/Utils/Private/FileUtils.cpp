#include "FileUtils.h"

#include <algorithm>

namespace fs = std::filesystem;

bool FileUtils::IsPathInsideProject(const fs::path& path) {
  try {
    fs::path absTarget = fs::absolute(path).lexically_normal();
    fs::path absProject = fs::absolute(fs::current_path()).lexically_normal();

    auto [root_it, target_it] =
        std::mismatch(absProject.begin(), absProject.end(), absTarget.begin(), absTarget.end());

    return root_it == absProject.end();
  } catch (...) {
    return false;
  }
}

std::string FileUtils::GetProjectRelativePath(const std::string& fullPath) {
  if (fullPath.empty()) return "";

  fs::path p(fullPath);
  return fs::relative(p, fs::current_path()).generic_string();
}
