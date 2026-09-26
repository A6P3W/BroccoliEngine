#include "NativeLibrary.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <system_error>
#include <utility>

namespace {
std::string GetWindowsErrorMessage(DWORD ErrorCode) {
  if (ErrorCode == ERROR_SUCCESS) return "Unknown Windows error.";

  LPSTR Buffer = nullptr;
  const DWORD Length = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      ErrorCode,
      0,
      reinterpret_cast<LPSTR>(&Buffer),
      0,
      nullptr
  );
  if (Length == 0 || Buffer == nullptr) {
    return "Windows error " + std::to_string(ErrorCode) + ".";
  }

  std::string Message(Buffer, Length);
  LocalFree(Buffer);
  while (!Message.empty() && (Message.back() == '\r' || Message.back() == '\n')) {
    Message.pop_back();
  }
  return Message;
}
}  // namespace

NativeLibrary::~NativeLibrary() { Unload(); }

NativeLibrary::NativeLibrary(NativeLibrary&& Other) noexcept
    : Handle(std::exchange(Other.Handle, nullptr)), LastError(std::move(Other.LastError)) {}

NativeLibrary& NativeLibrary::operator=(NativeLibrary&& Other) noexcept {
  if (this == &Other) return *this;

  Unload();
  Handle = std::exchange(Other.Handle, nullptr);
  LastError = std::move(Other.LastError);
  return *this;
}

bool NativeLibrary::Load(const std::filesystem::path& Path) {
  Unload();
  LastError.clear();

  HMODULE Module = nullptr;
#if defined(__MINGW32__)
  // GCC eagerly loads linked plugins. Reuse that module when plugin discovery runs.
  if (GetModuleHandleExW(0, Path.filename().c_str(), &Module) && Module != nullptr) {
    std::wstring LoadedPath(MAX_PATH, L'\0');
    DWORD Length =
        GetModuleFileNameW(Module, LoadedPath.data(), static_cast<DWORD>(LoadedPath.size()));
    while (Length == LoadedPath.size() && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      LoadedPath.resize(LoadedPath.size() * 2);
      Length = GetModuleFileNameW(Module, LoadedPath.data(), static_cast<DWORD>(LoadedPath.size()));
    }
    if (Length > 0) {
      LoadedPath.resize(Length);
      std::error_code ErrorCode;
      if (!std::filesystem::equivalent(Path, LoadedPath, ErrorCode)) {
        FreeLibrary(Module);
        Module = nullptr;
      }
    } else {
      FreeLibrary(Module);
      Module = nullptr;
    }
  }
#endif
  if (Module == nullptr) Module = LoadLibraryW(Path.c_str());
  if (Module == nullptr) {
    LastError = GetWindowsErrorMessage(::GetLastError());
    return false;
  }

  Handle = Module;
  return true;
}

void* NativeLibrary::GetSymbol(const char* Name) const {
  LastError.clear();
  if (Handle == nullptr) {
    LastError = "The library is not loaded.";
    return nullptr;
  }
  if (Name == nullptr || Name[0] == '\0') {
    LastError = "The symbol name is empty.";
    return nullptr;
  }

  FARPROC Symbol = GetProcAddress(static_cast<HMODULE>(Handle), Name);
  if (Symbol == nullptr) {
    LastError = GetWindowsErrorMessage(::GetLastError());
    return nullptr;
  }
  return reinterpret_cast<void*>(Symbol);
}

void NativeLibrary::Unload() {
  if (Handle != nullptr) {
    FreeLibrary(static_cast<HMODULE>(Handle));
    Handle = nullptr;
  }
}

bool NativeLibrary::IsLoaded() const { return Handle != nullptr; }

const std::string& NativeLibrary::GetLastError() const { return LastError; }
