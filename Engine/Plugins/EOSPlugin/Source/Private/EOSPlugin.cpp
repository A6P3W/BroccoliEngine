#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "EOSOnlinePlayManager.h"
#include "EOSTitleStorageManager.h"
#include "IPlugin.h"
#include "NetworkManager.h"
#include "PluginAPI.h"
#include "PluginContext.h"

std::unique_ptr<INetworkTransport> CreateEOSP2PTransport();

class EOSPluginInstance final : public IPlugin {
 public:
  const char* GetName() const override { return "EOSPlugin"; }
  uint32_t GetApiVersion() const override { return BROCCOLI_PLUGIN_API_VERSION; }
  bool OnLoad(PluginContext& Context) override {
    if (!Context.RegisterNetworkTransport("EOSP2P", CreateEOSP2PTransport)) return false;
    EOSCoreManager::GetInstance().InitializeOnlineServices();
    EOSOnlinePlayManager::GetInstance();
    return true;
  }
  void OnUpdate(float DeltaTime) override {
    if (EOSCoreManager::GetInstance().IsInitialized()) EOSCoreManager::GetInstance().Tick();
  }
  void OnUnload() override {
    EOSTitleStorageManager::GetInstance().Shutdown();
    EOSOnlinePlayManager::GetInstance().Shutdown();
    if (NetworkManager::GetInstance().GetTransportName() == "EOSP2P")
      NetworkManager::GetInstance().Stop();
    EOSLobbyManager::GetInstance().Shutdown();
    EOSAuthManager::GetInstance().Shutdown();
    for (int TickIndex = 0; TickIndex < 3; ++TickIndex) EOSCoreManager::GetInstance().Tick();
    EOSCoreManager::GetInstance().Shutdown();
  }
};

extern "C" __declspec(dllexport) IPlugin* CreateBroccoliPlugin() { return new EOSPluginInstance(); }
extern "C" __declspec(dllexport) void DestroyBroccoliPlugin(IPlugin* Plugin) { delete Plugin; }
