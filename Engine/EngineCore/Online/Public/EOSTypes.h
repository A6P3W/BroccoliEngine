#pragma once

#include <string>

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

struct FLobbyInfo {
  std::string LobbyId;

  int CurrentMembers = 0;
  int MaxMembers = 0;

  bool bValid = false;
};

struct FCreateLobbyRequest {
  int MaxMembers = 4;
  bool bPublicAdvertised = true;
};

struct FLobbySearchRequest {
  int MaxResults = 10;
};
