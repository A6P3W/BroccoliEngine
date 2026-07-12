#include <windows.h>
#include <string>
#include <filesystem>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    wchar_t Path[MAX_PATH];
    GetModuleFileNameW(NULL, Path, MAX_PATH);
    std::filesystem::path ExePath(Path);
    
    // Path to the real game executable (Binaries/Launcher-game.exe)
    std::filesystem::path RealExe = ExePath.parent_path() / "Binaries" / "Launcher-game.exe";

    STARTUPINFOW Si = { sizeof(Si) };
    PROCESS_INFORMATION Pi;
    
    // Forward command line arguments
    std::wstring CmdLine = L"\"" + RealExe.wstring() + L"\" " + GetCommandLineW();

    // Keep working directory as root for resources
    std::filesystem::path WorkDir = ExePath.parent_path();

    if (CreateProcessW(
        RealExe.c_str(),
        &CmdLine[0],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        WorkDir.c_str(),
        &Si,
        &Pi
    )) {
        // Wait for game exit and return its exit code
        WaitForSingleObject(Pi.hProcess, INFINITE);
        DWORD ExitCode = 0;
        GetExitCodeProcess(Pi.hProcess, &ExitCode);
        
        CloseHandle(Pi.hProcess);
        CloseHandle(Pi.hThread);
        return static_cast<int>(ExitCode);
    }
    
    return 1;
}
