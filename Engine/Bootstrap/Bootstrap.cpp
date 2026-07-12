#include <shlwapi.h>
#include <windows.h>

#include <filesystem>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  wchar_t Path[MAX_PATH];
  if (GetModuleFileNameW(NULL, Path, MAX_PATH) == 0) return 1;

  std::filesystem::path ExePath(Path);

  std::filesystem::path RootDir = ExePath.parent_path();

  std::filesystem::path RealExe = RootDir / L"Binaries" / ExePath.filename();

  if (!std::filesystem::exists(RealExe)) {
    MessageBoxW(
        NULL,
        (L"Executable not found:\n" + RealExe.wstring()).c_str(),
        L"Launch Error",
        MB_ICONERROR
    );
    return 1;
  }

  LPWSTR CmdArgs = PathGetArgsW(GetCommandLineW());
  std::wstring CombinedCmd = L"\"" + RealExe.wstring() + L"\" " + CmdArgs;

  STARTUPINFOW Si = {sizeof(Si)};
  PROCESS_INFORMATION Pi;
  ZeroMemory(&Si, sizeof(Si));
  Si.cb = sizeof(Si);
  ZeroMemory(&Pi, sizeof(Pi));

  std::wstring WorkDir = RootDir.wstring();

  if (CreateProcessW(
          RealExe.c_str(), &CombinedCmd[0], NULL, NULL, FALSE, 0, NULL, WorkDir.c_str(), &Si, &Pi
      )) {
    CloseHandle(Pi.hProcess);
    CloseHandle(Pi.hThread);
    return 0;
  } else {
    DWORD Error = GetLastError();
    MessageBoxW(
        NULL,
        (L"Failed to launch process. Error code: " + std::to_wstring(Error)).c_str(),
        L"Launch Error",
        MB_ICONERROR
    );
    return 1;
  }
}
