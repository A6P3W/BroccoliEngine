#pragma once
#include <eos_connect.h>
#include <eos_lobby.h>
#include <eos_p2p.h>
#include <eos_sdk.h>
#include <eos_titlestorage_types.h>

#include <cstdint>

#include "BroccoliEngineAPI.h"
#include "EOSTypes.h"

class BROCCOLI_ENGINE_API EOSCoreManager {
 public:
  static EOSCoreManager& GetInstance();

  bool InitializeOnlineServices(const char* ProductVersion = "0.1.0");

  bool Initialize(const FEOSConfig& Config);
  void Tick();
  void Shutdown();
  bool IsInitialized() const;
  EOS_HPlatform GetPlatformHandle() const;
  EOS_HConnect GetConnectHandle() const;
  EOS_HLobby GetLobbyHandle() const;
  EOS_HP2P GetP2PHandle() const;
  EOS_HTitleStorage GetTitleStorageHandle() const;

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
