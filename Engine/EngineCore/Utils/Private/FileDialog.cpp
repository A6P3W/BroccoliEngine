#include "FileDialog.h"

#include <DxLib.h>
#include <Windows.h>
#include <commdlg.h>

#include <filesystem>
#include <system_error>

namespace {
std::string GetDefaultInitialDir() {
  const std::string currentPath = std::filesystem::current_path().string();
  const std::string resourceDir = "Resources/" + currentPath;
  std::error_code errorCode;
  return std::filesystem::exists(resourceDir, errorCode) ? resourceDir : currentPath;
}
}  // namespace

std::string FileDialog::OpenFile(const char* filter) {
  std::string ResourceDir = GetDefaultInitialDir();

  OPENFILENAMEA ofn;
  CHAR szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = GetMainWindowHandle();
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = ResourceDir.c_str();
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn) == TRUE) {
    return std::string(ofn.lpstrFile);
  }
  return std::string();
}

std::string FileDialog::SaveFile(const char* filter, const char* defaultExt) {
  std::string ResourceDir = GetDefaultInitialDir();

  OPENFILENAMEA ofn;
  CHAR szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = GetMainWindowHandle();
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = ResourceDir.c_str();
  ofn.lpstrDefExt = defaultExt;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

  if (GetSaveFileNameA(&ofn) == TRUE) {
    return std::string(ofn.lpstrFile);
  }
  return std::string();
}
