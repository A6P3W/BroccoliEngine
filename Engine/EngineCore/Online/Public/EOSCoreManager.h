#pragma once

#include <eos_connect.h>
#include <eos_lobby.h>
#include <eos_sdk.h>

#include <cstdint>

#include "EOSTypes.h"

class EOSCoreManager {
 public:
  static EOSCoreManager& Get();
  template <class TCredentials>
  FEOSConfig MakeEOSConfig(const char* ProductVersion = "0.1.0") {
    FEOSConfig Config = {};
    Config.ProductName = TCredentials::GameName;
    Config.ProductVersion = ProductVersion;
    Config.ProductId = TCredentials::ProductId;
    Config.SandboxId = TCredentials::SandboxId;
    Config.DeploymentId = TCredentials::DeploymentId;
    Config.ClientId = TCredentials::ClientCredentialsId;
    Config.ClientSecret = TCredentials::ClientCredentialsSecret;
    Config.EncryptionKey = TCredentials::EncryptionKey;
    return Config;
  }

  template <class TCredentials>
  bool InitializeOnlineServices(const char* ProductVersion = "0.1.0") {
    return EOSCoreManager::Get().Initialize(MakeEOSConfig<TCredentials>(ProductVersion));
  }

  bool Initialize(const FEOSConfig& Config);
  void Tick();
  void Shutdown();

  bool IsInitialized() const;

  EOS_HPlatform GetPlatformHandle() const;
  EOS_HConnect GetConnectHandle() const;
  EOS_HLobby GetLobbyHandle() const;

 private:
  EOSCoreManager() = default;
  ~EOSCoreManager() = default;

  EOSCoreManager(const EOSCoreManager&) = delete;
  EOSCoreManager& operator=(const EOSCoreManager&) = delete;

 private:
  EOS_HPlatform PlatformHandle = nullptr;
  bool bInitialized = false;
  bool bSDKInitialized = false;
  bool bTickLogged = false;
  uint64_t TickCount = 0;
};
