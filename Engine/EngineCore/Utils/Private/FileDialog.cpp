#include "FileDialog.h"

#include <DxLib.h>
#include <Windows.h>
#include <commdlg.h>
#include <shobjidl.h>

#include <filesystem>
#include <system_error>

namespace {
std::string WideToUtf8(const std::wstring& Value) {
  if (Value.empty()) {
    return {};
  }

  const int Length = WideCharToMultiByte(
      CP_UTF8, 0, Value.data(), static_cast<int>(Value.size()), nullptr, 0, nullptr, nullptr
  );
  if (Length <= 0) {
    return {};
  }

  std::string Result(static_cast<size_t>(Length), '\0');
  WideCharToMultiByte(
      CP_UTF8,
      0,
      Value.data(),
      static_cast<int>(Value.size()),
      Result.data(),
      Length,
      nullptr,
      nullptr
  );
  return Result;
}

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

std::string FileDialog::SelectFolder() {
  const HRESULT InitializeResult =
      CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  const bool bShouldUninitialize = SUCCEEDED(InitializeResult);
  if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) {
    return {};
  }

  IFileOpenDialog* Dialog = nullptr;
  HRESULT Result =
      CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog));
  std::string SelectedPath;
  if (SUCCEEDED(Result)) {
    FILEOPENDIALOGOPTIONS Options = 0;
    Result = Dialog->GetOptions(&Options);
    if (SUCCEEDED(Result)) {
      Result =
          Dialog->SetOptions(Options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
    }

    if (SUCCEEDED(Result)) {
      Result = Dialog->Show(GetMainWindowHandle());
    }
    if (SUCCEEDED(Result)) {
      IShellItem* SelectedItem = nullptr;
      Result = Dialog->GetResult(&SelectedItem);
      if (SUCCEEDED(Result)) {
        PWSTR WidePath = nullptr;
        Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WidePath);
        if (SUCCEEDED(Result) && WidePath != nullptr) {
          SelectedPath = WideToUtf8(WidePath);
          CoTaskMemFree(WidePath);
        }
        SelectedItem->Release();
      }
    }
    Dialog->Release();
  }

  if (bShouldUninitialize) {
    CoUninitialize();
  }
  return SelectedPath;
}
