#include "PathResolver.h"

#include <cctype>
#include <filesystem>
#include <string>

#include "EngineDefine.h"
#include "FileUtils.h"
#include "Log.h"

namespace {
std::string NormalizePath(const std::string& Path) {
  std::string Result = Path;
  for (char& Ch : Result) {
    if (Ch == '\\') Ch = '/';
  }
  return Result;
}

bool EqualsCaseInsensitiveAt(const std::string& FullStr, size_t Offset, const std::string& Value) {
  if (FullStr.length() - Offset < Value.length()) return false;
  for (size_t Index = 0; Index < Value.length(); ++Index) {
    if (std::tolower(static_cast<unsigned char>(FullStr[Offset + Index])) !=
        std::tolower(static_cast<unsigned char>(Value[Index]))) {
      return false;
    }
  }
  return true;
}

bool StartsWithCaseInsensitive(const std::string& FullStr, const std::string& Prefix) {
  return EqualsCaseInsensitiveAt(FullStr, 0, Prefix);
}

size_t FindCaseInsensitive(const std::string& FullStr, const std::string& Value) {
  if (Value.empty() || FullStr.length() < Value.length()) return std::string::npos;

  for (size_t Start = 0; Start <= FullStr.length() - Value.length(); ++Start) {
    if (EqualsCaseInsensitiveAt(FullStr, Start, Value)) return Start;
  }
  return std::string::npos;
}

bool IsInsideBase(const std::filesystem::path& RelPath) {
  if (RelPath.empty()) return false;
  const auto FirstComp = *RelPath.begin();
  return FirstComp != "..";
}

std::string GGameName =
#ifdef BROCCOLI_GAME_NAME
    BROCCOLI_GAME_NAME;
#else
    "";
#endif

std::string GGameSourceDir;
std::string GProjectRoot =
#ifdef BROCCOLI_PROJECT_ROOT
    NormalizePath(BROCCOLI_PROJECT_ROOT);
#else
    "";
#endif

std::string DetectGameSourceDir() {
#ifdef BROCCOLI_GAME_SOURCE_DIR
  std::string Dir = NormalizePath(BROCCOLI_GAME_SOURCE_DIR);
  if (!Dir.empty() && Dir.back() != '/') Dir += '/';
  return Dir;
#else
#ifdef BROCCOLI_ENGINE_RESOURCE_DIR
  std::error_code ec;
  std::filesystem::path EngineResPath =
      FileUtils::Utf8ToPath(NormalizePath(BROCCOLI_ENGINE_RESOURCE_DIR)).lexically_normal();
  std::filesystem::path SolutionRoot = EngineResPath.parent_path().parent_path();
  std::filesystem::path GamePath = SolutionRoot / PathResolver::GetGameName();
  if (std::filesystem::exists(GamePath, ec)) {
    std::string D = NormalizePath(FileUtils::PathToUtf8(GamePath));
    if (!D.empty() && D.back() != '/') D += '/';
    return D;
  }
#endif
  std::error_code ec2;
  std::filesystem::path Curr = std::filesystem::current_path(ec2);
  for (int i = 0; i < 5 && !Curr.empty(); ++i) {
    std::filesystem::path Candidate = Curr / PathResolver::GetGameName();
    if (std::filesystem::exists(Candidate, ec2)) {
      std::string D = NormalizePath(FileUtils::PathToUtf8(Candidate));
      if (!D.empty() && D.back() != '/') D += '/';
      return D;
    }
    Curr = Curr.parent_path();
  }
  return "";
#endif
}
}  // namespace

void PathResolver::SetProjectRoot(const std::string& Root) {
  GProjectRoot = NormalizePath(Root);
  while (GProjectRoot.length() > 1 && GProjectRoot.back() == '/') {
    GProjectRoot.pop_back();
  }
  GGameSourceDir.clear();
}

const std::string& PathResolver::GetProjectRoot() { return GProjectRoot; }

void PathResolver::SetGameName(const std::string& Name) {
  GGameName = Name;
  GGameSourceDir.clear();
}

const std::string& PathResolver::GetGameName() {
  if (GGameName.empty()) {
    std::error_code ErrorCode;
    if (std::filesystem::exists("Resources", ErrorCode)) {
      for (const auto& Entry : std::filesystem::directory_iterator("Resources", ErrorCode)) {
        if (Entry.is_directory(ErrorCode)) {
          std::string FolderName = FileUtils::PathToUtf8(Entry.path().filename());
          if (FolderName != "Engine" && !FolderName.empty() && FolderName[0] != '.') {
            GGameName = FolderName;
            break;
          }
        }
      }
    }
    if (GGameName.empty()) {
      GGameName = "Game";
    }
  }
  return GGameName;
}

std::string PathResolver::SanitizeResourcePath(const std::string& Path) {
  if (Path.empty()) return "";

  std::string CleanPath = NormalizePath(Path);

  // 先頭の連続スラッシュを整形
  while (CleanPath.length() > 1 && CleanPath[0] == '/' && CleanPath[1] == '/') {
    CleanPath.erase(0, 1);
  }

  // 既に /Engine/ または Engine/ または /Game/ または Game/ で始まっている場合
  if (CleanPath.rfind("/Engine/", 0) == 0) {
    return CleanPath;
  }
  if (CleanPath.rfind("Engine/", 0) == 0) {
    return "/" + CleanPath;
  }
  if (CleanPath.rfind("/Game/", 0) == 0) {
    return CleanPath;
  }
  if (CleanPath.rfind("Game/", 0) == 0) {
    return "/" + CleanPath;
  }

  // 絶対パスの場合の判定
  std::error_code ErrorCode;
  std::filesystem::path PathObj = FileUtils::Utf8ToPath(CleanPath);
  if (PathObj.is_absolute()) {
    std::string EngineDir = NormalizePath(GetEngineResourceDir());
    std::string GameDir = NormalizePath(GetGameResourceDir());

    std::filesystem::path AbsEnginePath =
        std::filesystem::absolute(FileUtils::Utf8ToPath(EngineDir), ErrorCode);
    std::string AbsEngineStr = ErrorCode ? "" : NormalizePath(FileUtils::PathToUtf8(AbsEnginePath));
    if (!AbsEngineStr.empty() && AbsEngineStr.back() != '/') AbsEngineStr += '/';

    ErrorCode.clear();
    std::filesystem::path AbsGamePath =
        std::filesystem::absolute(FileUtils::Utf8ToPath(GameDir), ErrorCode);
    std::string AbsGameStr = ErrorCode ? "" : NormalizePath(FileUtils::PathToUtf8(AbsGamePath));
    if (!AbsGameStr.empty() && AbsGameStr.back() != '/') AbsGameStr += '/';

    if (!EngineDir.empty() && EngineDir.back() != '/') EngineDir += '/';
    if (!GameDir.empty() && GameDir.back() != '/') GameDir += '/';

    if (StartsWithCaseInsensitive(CleanPath, EngineDir)) {
      return "/Engine/" + CleanPath.substr(EngineDir.length());
    }
    if (!AbsEngineStr.empty() && StartsWithCaseInsensitive(CleanPath, AbsEngineStr)) {
      return "/Engine/" + CleanPath.substr(AbsEngineStr.length());
    }

    if (StartsWithCaseInsensitive(CleanPath, GameDir)) {
      return "/Game/" + CleanPath.substr(GameDir.length());
    }
    if (!AbsGameStr.empty() && StartsWithCaseInsensitive(CleanPath, AbsGameStr)) {
      return "/Game/" + CleanPath.substr(AbsGameStr.length());
    }

    const std::string LegacyGameResourceMarker = "/Resources/" + GetGameName() + "/";
    const size_t LegacyGameResourcePos = FindCaseInsensitive(CleanPath, LegacyGameResourceMarker);
    if (LegacyGameResourcePos != std::string::npos &&
        FindCaseInsensitive(CleanPath.substr(0, LegacyGameResourcePos), "/Bin/") !=
            std::string::npos) {
      return "/Game/" + CleanPath.substr(LegacyGameResourcePos + LegacyGameResourceMarker.length());
    }

    // std::filesystem::relative によるフォールバック判定
    if (!AbsEngineStr.empty()) {
      ErrorCode.clear();
      std::filesystem::path RelEngine =
          std::filesystem::relative(PathObj, FileUtils::Utf8ToPath(AbsEngineStr), ErrorCode);
      if (!ErrorCode && IsInsideBase(RelEngine)) {
        return "/Engine/" + NormalizePath(FileUtils::PathToUtf8Generic(RelEngine));
      }
    }

    if (!AbsGameStr.empty()) {
      ErrorCode.clear();
      std::filesystem::path RelGame =
          std::filesystem::relative(PathObj, FileUtils::Utf8ToPath(AbsGameStr), ErrorCode);
      if (!ErrorCode && IsInsideBase(RelGame)) {
        return "/Game/" + NormalizePath(FileUtils::PathToUtf8Generic(RelGame));
      }
    }

    return CleanPath;
  }

  // 明示的な仮想プレフィックスを持たない相対パスは、通常の相対パスとして保持する。
  return CleanPath;
}

std::string PathResolver::Resolve(const std::string& Path) {
  if (Path.empty()) return "";

  const std::filesystem::path PathObj = FileUtils::Utf8ToPath(Path);
  if (PathObj.is_absolute()) {
    std::string Normalized = NormalizePath(Path);
#if defined(_DEBUG)
    M_LOG("[PathResolver] Resolve (absolute): input='{}' -> result='{}'", Path, Normalized);
#endif
    return Normalized;
  }

  std::string Sanitized = SanitizeResourcePath(Path);
  std::string Result;

  if (Sanitized.rfind("/Engine/", 0) == 0) {
    std::string SubPath = Sanitized.substr(8);
    Result = GetEngineResourceDir() + SubPath;
  } else if (Sanitized.rfind("/Game/", 0) == 0) {
    std::string SubPath = Sanitized.substr(6);
    Result = GetGameResourceDir() + SubPath;
  } else {
    Result = Sanitized;
  }

#if defined(_DEBUG)
  M_LOG(
      "[PathResolver] Resolve: input='{}' sanitized='{}' -> result='{}'", Path, Sanitized, Result
  );
#endif
  return Result;
}

std::string PathResolver::GetEngineResourceDir() {
  if constexpr (IsEditor) {
#ifdef BROCCOLI_ENGINE_RESOURCE_DIR
    static std::string Dir = []() {
      std::string D = NormalizePath(BROCCOLI_ENGINE_RESOURCE_DIR);
      if (!D.empty() && D.back() != '/') D += '/';
      return D;
    }();
    return Dir;
#else
    return "Resources/Engine/";
#endif
  } else {
    return "Resources/Engine/";
  }
}

std::string PathResolver::GetGameResourceDir() {
  if constexpr (IsEditor) {
    if (!GGameSourceDir.empty()) return GGameSourceDir + "Resources/";

    if (!GProjectRoot.empty()) {
      return GProjectRoot + "/" + GetGameName() + "/Resources/";
    }

    std::string GameDir = DetectGameSourceDir();
    if (!GameDir.empty()) {
      GGameSourceDir = GameDir;
      return GameDir + "Resources/";
    }
    return "Resources/";
  } else {
    std::string Dir = "Resources/" + GetGameName() + "/";
    return Dir;
  }
}

void PathResolver::InitializeWorkingDirectory() {
  if constexpr (IsEditor) {
    if (!GProjectRoot.empty()) {
      const std::filesystem::path GameRoot =
          FileUtils::Utf8ToPath(GProjectRoot) / FileUtils::Utf8ToPath(GetGameName());
      std::error_code ErrorCode;
      const bool IsGameRootDirectory = std::filesystem::is_directory(GameRoot, ErrorCode);
      if (ErrorCode) {
        M_LOG(
            "Failed to inspect game directory {}: {}",
            FileUtils::PathToUtf8(GameRoot),
            ErrorCode.message()
        );
      } else if (!IsGameRootDirectory) {
        M_LOG(
            "Game directory was not found under project root: {}", FileUtils::PathToUtf8(GameRoot)
        );
      } else {
        ErrorCode.clear();
        std::filesystem::current_path(GameRoot, ErrorCode);
        if (!ErrorCode) {
          GGameSourceDir = NormalizePath(FileUtils::PathToUtf8(GameRoot));
          if (!GGameSourceDir.empty() && GGameSourceDir.back() != '/') {
            GGameSourceDir += '/';
          }
          M_LOG("Working directory set to game folder: {}", GGameSourceDir);
          M_LOG("Game resource directory: {}", GetGameResourceDir());
          return;
        }
        M_LOG(
            "Failed to set working directory to game folder {}: {}",
            FileUtils::PathToUtf8(GameRoot),
            ErrorCode.message()
        );
      }
    }

    std::string GameSourceDir = DetectGameSourceDir();
    if (!GameSourceDir.empty()) {
      GGameSourceDir = GameSourceDir;
      std::error_code ErrorCode;
      std::filesystem::current_path(FileUtils::Utf8ToPath(GameSourceDir), ErrorCode);
      if (ErrorCode) {
        M_LOG("Failed to set working directory to {}: {}", GameSourceDir, ErrorCode.message());
      } else {
        M_LOG("Working directory set to game folder: {}", GameSourceDir);
      }
    } else {
      M_LOG("Could not detect GameSourceDir. Keeping current working directory.");
    }
  }
}
