#include "FileUtils.h"
#include "UMath.h"

#include <algorithm>

namespace fs = std::filesystem;

struct FileUtils::Impl {
  bool IsPathInsideProject(const fs::path& path) const {
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

  std::string GetProjectRelativePath(const std::string& fullPath) const {
    if (fullPath.empty()) return "";

    fs::path p(fullPath);
    return fs::relative(p, fs::current_path()).generic_string();
  }
};

FileUtils::Impl* FileUtils::GetImpl() {
  static Impl Instance;
  return &Instance;
}

bool FileUtils::IsPathInsideProject(const fs::path& path) {
  return GetImpl()->IsPathInsideProject(path);
}

std::string FileUtils::GetProjectRelativePath(const std::string& fullPath) {
  return GetImpl()->GetProjectRelativePath(fullPath);
}

BROCCOLI_ENGINE_API void DummyForceExport() {
  FVector2D V(1.0f, 1.0f);
  FVector2D V2 = V * 1.0f;
  FVector2D V3 = FVector2D::ZeroVector();
  (void)V2;
  (void)V3;
}
