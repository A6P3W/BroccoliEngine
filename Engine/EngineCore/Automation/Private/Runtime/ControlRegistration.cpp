#include "ControlRegistration.h"

#include <Windows.h>

#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "Log.h"

namespace {
std::filesystem::path GetExecutablePath() {
  std::vector<wchar_t> Buffer(MAX_PATH);
  while (true) {
    const DWORD Length =
        GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
    if (Length == 0) {
      return {};
    }
    if (Length < Buffer.size() - 1) {
      return std::filesystem::path(std::wstring(Buffer.data(), Length));
    }
    Buffer.resize(Buffer.size() * 2);
  }
}
}  // namespace

bool FAutomationControlRegistration::Create(uint16_t Port) {
  const std::filesystem::path ExecutablePath = GetExecutablePath();
  if (ExecutablePath.empty()) {
    M_LOG(Log, "Automation control registration could not resolve the executable path.");
    return false;
  }

  const DWORD ProcessId = GetCurrentProcessId();
  const std::filesystem::path ControlDirectory = ExecutablePath.parent_path() / "control";
  const std::filesystem::path NewRegistrationPath =
      ControlDirectory / (std::to_string(ProcessId) + ".json");
  std::error_code Error;
  std::filesystem::create_directories(ControlDirectory, Error);
  if (Error) {
    M_LOG(Log, "Automation control registration could not create '{}'.", ControlDirectory.string());
    return false;
  }

  std::ofstream RegistrationFile(NewRegistrationPath, std::ios::out | std::ios::trunc);
  if (!RegistrationFile) {
    M_LOG(
        Log, "Automation control registration could not create '{}'.", NewRegistrationPath.string()
    );
    return false;
  }
  RegistrationFile << "{\n  \"pid\": " << ProcessId << ",\n  \"port\": " << Port << "\n}\n";
  RegistrationFile.close();
  if (!RegistrationFile) {
    std::filesystem::remove(NewRegistrationPath, Error);
    M_LOG(
        Log, "Automation control registration could not write '{}'.", NewRegistrationPath.string()
    );
    return false;
  }

  RegistrationPath = NewRegistrationPath;
  M_LOG(Log, "Automation control registered PID {} on port {}.", ProcessId, Port);
  return true;
}

void FAutomationControlRegistration::Remove() {
  if (RegistrationPath.empty()) {
    return;
  }

  std::error_code Error;
  std::filesystem::remove(RegistrationPath, Error);
  if (Error) {
    M_LOG(Log, "Automation control registration could not remove '{}'.", RegistrationPath.string());
  }
  RegistrationPath.clear();
}
