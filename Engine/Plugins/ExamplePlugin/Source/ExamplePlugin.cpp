#include "IPlugin.h"
#include "PluginAPI.h"
#include "PluginContext.h"

class ExamplePlugin final : public IPlugin {
 public:
  const char* GetName() const override { return "ExamplePlugin"; }

  uint32_t GetApiVersion() const override { return BROCCOLI_PLUGIN_API_VERSION; }

  bool OnLoad(PluginContext& Context) override {
    Context.LogInfo("ExamplePlugin loaded.");
    return true;
  }

  void OnUnload() override { Context.LogInfo("ExamplePlugin unloaded."); }

  void OnUpdate(float DeltaTime) override {
    ElapsedTime += DeltaTime;
    if (!LoggedFirstUpdate) {
      Context.LogInfo("ExamplePlugin updated.");
      LoggedFirstUpdate = true;
    }
  }

 private:
  PluginContext Context;
  float ElapsedTime = 0.0f;
  bool LoggedFirstUpdate = false;
};

extern "C" __declspec(dllexport) IPlugin* CreateBroccoliPlugin() { return new ExamplePlugin(); }

extern "C" __declspec(dllexport) void DestroyBroccoliPlugin(IPlugin* Plugin) { delete Plugin; }
