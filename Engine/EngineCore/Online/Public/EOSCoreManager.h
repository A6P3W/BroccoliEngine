#pragma once
#include "BroccoliEngineAPI.h"
#include <eos_connect.h>
#include <eos_lobby.h>
#include <eos_sdk.h>

#include <cstdint>

#include "EOSTypes.h"

class BROCCOLI_ENGINE_API EOSCoreManager {
 public:
  static EOSCoreManager& Get();

  bool InitializeOnlineServices(const char* ProductVersion = "0.1.0");

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
