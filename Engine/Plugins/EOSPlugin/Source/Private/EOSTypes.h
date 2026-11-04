#pragma once
#include <EOSPlugin/EOSPlugin.h>
using namespace BEPlugins::EOS;

using ELobbyDisconnectReason = ESessionDisconnectReason;
enum class EAuthLossReason { TokenExpired, NetworkError, Unknown };
struct FEOSConfig {
  const char* ProductName = nullptr;
  const char* ProductVersion = nullptr;
  const char* ProductId = nullptr;
  const char* SandboxId = nullptr;
  const char* DeploymentId = nullptr;
  const char* ClientId = nullptr;
  const char* ClientSecret = nullptr;
  const char* EncryptionKey = nullptr;
};
