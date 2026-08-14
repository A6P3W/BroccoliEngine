#include "FileDialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#include <Windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

#include <wrl/client.h>

#include <filesystem>
#include <system_error>
#include <vector>

#include "BroccoliRaylib.h"
#include "FileUtils.h"
#include "Log.h"
#include "PathResolver.h"

using Microsoft::WRL::ComPtr;

namespace {
struct ScopeCoUninitialize {
  bool bActive = false;
  ~ScopeCoUninitialize() {
    if (bActive) {
      CoUninitialize();
    }
  }
};

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

std::wstring Utf8ToWide(const std::string& Value) {
  if (Value.empty()) {
    return {};
  }
  const int Length =
      MultiByteToWideChar(CP_UTF8, 0, Value.data(), static_cast<int>(Value.size()), nullptr, 0);
  if (Length <= 0) {
    return {};
  }
  std::wstring Result(static_cast<size_t>(Length), L'\0');
  MultiByteToWideChar(
      CP_UTF8, 0, Value.data(), static_cast<int>(Value.size()), Result.data(), Length
  );
  return Result;
}

std::string GetDefaultInitialDir() {
  const std::string GameResourceDir = PathResolver::GetGameResourceDir();
  if (!GameResourceDir.empty()) {
    std::error_code ErrorCode;
    if (std::filesystem::exists(FileUtils::Utf8ToPath(GameResourceDir), ErrorCode)) {
      return GameResourceDir;
    }
  }

  std::error_code ErrorCode;
  const std::filesystem::path CurrentPath = std::filesystem::current_path(ErrorCode);
  if (ErrorCode) return {};

  const std::filesystem::path ResourceDir = CurrentPath / "Resources";
  if (std::filesystem::exists(ResourceDir, ErrorCode)) {
    return FileUtils::PathToUtf8(ResourceDir);
  }
  return FileUtils::PathToUtf8(CurrentPath);
}

std::vector<COMDLG_FILTERSPEC> ParseFilterSpecs(
    const char* Filter, std::vector<std::wstring>& FilterStrings
) {
  std::vector<COMDLG_FILTERSPEC> FilterSpecs;
  if (Filter == nullptr) return FilterSpecs;

  const char* Current = Filter;
  while (*Current != '\0') {
    const std::string DisplayName(Current);
    Current += DisplayName.size() + 1;
    if (*Current == '\0') break;

    const std::string Pattern(Current);
    Current += Pattern.size() + 1;
    FilterStrings.push_back(Utf8ToWide(DisplayName));
    FilterStrings.push_back(Utf8ToWide(Pattern));
  }

  FilterSpecs.reserve(FilterStrings.size() / 2);
  for (size_t Index = 0; Index + 1 < FilterStrings.size(); Index += 2) {
    FilterSpecs.push_back({FilterStrings[Index].c_str(), FilterStrings[Index + 1].c_str()});
  }
  return FilterSpecs;
}

void LogInitialDirectory(const char* DialogName, const std::string& ResourceDir) {
  std::error_code ErrorCode;
  const bool bDirectoryExists =
      std::filesystem::is_directory(FileUtils::Utf8ToPath(ResourceDir), ErrorCode);
  M_LOG(
      Log,
      "[FileDialog] {} initialDir='{}' isDirectory={} error={}",
      DialogName,
      ResourceDir,
      bDirectoryExists,
      ErrorCode.value()
  );
}

std::wstring GetDialogFolderPath(const std::string& ResourceDir) {
  std::error_code ErrorCode;
  const std::filesystem::path AbsolutePath =
      std::filesystem::absolute(FileUtils::Utf8ToPath(ResourceDir), ErrorCode).lexically_normal();
  if (ErrorCode) return {};
  return AbsolutePath.wstring();
}

template <typename TDialogInterface>
std::string ShowFileDialog(
    const CLSID& DialogClsid,
    const IID& DialogIid,
    const char* Filter,
    const char* DefaultExt,
    FILEOPENDIALOGOPTIONS ExtraOptions,
    const char* DialogName,
    const std::string& InitialDir
) {
  const std::string ResourceDir = InitialDir.empty() ? GetDefaultInitialDir() : InitialDir;
  const std::wstring WideResourceDir = GetDialogFolderPath(ResourceDir);
  const std::wstring WideDefaultExt = DefaultExt != nullptr ? Utf8ToWide(DefaultExt) : L"";
  LogInitialDirectory(DialogName, ResourceDir);

  const HRESULT InitializeResult =
      CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  ScopeCoUninitialize ScopeGuard{SUCCEEDED(InitializeResult)};
  if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) return {};

  ComPtr<TDialogInterface> Dialog;
  std::string SelectedPath;
  HRESULT Result = CoCreateInstance(DialogClsid, nullptr, CLSCTX_INPROC_SERVER, DialogIid, &Dialog);
  if (SUCCEEDED(Result)) {
    FILEOPENDIALOGOPTIONS Options = 0;
    Result = Dialog->GetOptions(&Options);
    if (SUCCEEDED(Result)) {
      Result = Dialog->SetOptions(Options | ExtraOptions);
    }

    std::vector<std::wstring> FilterStrings;
    const std::vector<COMDLG_FILTERSPEC> FilterSpecs = ParseFilterSpecs(Filter, FilterStrings);
    if (SUCCEEDED(Result) && !FilterSpecs.empty()) {
      Result = Dialog->SetFileTypes(static_cast<UINT>(FilterSpecs.size()), FilterSpecs.data());
    }

    if (SUCCEEDED(Result) && !WideDefaultExt.empty()) {
      ComPtr<IFileSaveDialog> SaveDialog;
      if (SUCCEEDED(Dialog.As(&SaveDialog))) {
        SaveDialog->SetDefaultExtension(WideDefaultExt.c_str());
      }
    }

    ComPtr<IShellItem> InitialFolder;
    if (SUCCEEDED(Result) && !WideResourceDir.empty()) {
      const HRESULT FolderResult = SHCreateItemFromParsingName(
          WideResourceDir.c_str(), nullptr, IID_PPV_ARGS(&InitialFolder)
      );
      M_LOG(Log, "[FileDialog] {} SHCreateItemFromParsingName result={}", DialogName, FolderResult);
      if (SUCCEEDED(FolderResult)) {
        const HRESULT SetFolderResult = Dialog->SetFolder(InitialFolder.Get());
        M_LOG(Log, "[FileDialog] {} SetFolder result={}", DialogName, SetFolderResult);
      }
    }

    if (SUCCEEDED(Result)) {
      Result = Dialog->Show(reinterpret_cast<HWND>(GetWindowHandle()));
    }
    if (SUCCEEDED(Result)) {
      ComPtr<IShellItem> SelectedItem;
      Result = Dialog->GetResult(&SelectedItem);
      if (SUCCEEDED(Result) && SelectedItem != nullptr) {
        PWSTR WidePath = nullptr;
        Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WidePath);
        if (SUCCEEDED(Result) && WidePath != nullptr) {
          SelectedPath = WideToUtf8(WidePath);
          CoTaskMemFree(WidePath);
        }
      }
    }
  }

  return SelectedPath;
}
}  // namespace

std::string FileDialog::OpenFile(const char* Filter, const std::string& InitialDir) {
  return ShowFileDialog<IFileOpenDialog>(
      CLSID_FileOpenDialog,
      IID_IFileOpenDialog,
      Filter,
      nullptr,
      FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR,
      "OpenFile",
      InitialDir
  );
}

std::string FileDialog::SaveFile(
    const char* Filter, const char* DefaultExt, const std::string& InitialDir
) {
  return ShowFileDialog<IFileSaveDialog>(
      CLSID_FileSaveDialog,
      IID_IFileSaveDialog,
      Filter,
      DefaultExt,
      FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_NOCHANGEDIR,
      "SaveFile",
      InitialDir
  );
}

std::string FileDialog::SelectFolder() {
  const HRESULT InitializeResult =
      CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  ScopeCoUninitialize ScopeGuard{SUCCEEDED(InitializeResult)};
  if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) {
    return {};
  }

  ComPtr<IFileOpenDialog> Dialog;
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
      Result = Dialog->Show(reinterpret_cast<HWND>(GetWindowHandle()));
    }
    if (SUCCEEDED(Result)) {
      ComPtr<IShellItem> SelectedItem;
      Result = Dialog->GetResult(&SelectedItem);
      if (SUCCEEDED(Result) && SelectedItem != nullptr) {
        PWSTR WidePath = nullptr;
        Result = SelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &WidePath);
        if (SUCCEEDED(Result) && WidePath != nullptr) {
          SelectedPath = WideToUtf8(WidePath);
          CoTaskMemFree(WidePath);
        }
      }
    }
  }

  return SelectedPath;
}
