#include "ExamplePlugin/ExamplePlugin.h"

#include <random>

#include "IPlugin.h"
#include "PluginAPI.h"
#include "PluginContext.h"

namespace ExamplePlugin {
int GetRandomNumber() {
  thread_local std::mt19937 Generator(std::random_device{}());
  thread_local std::uniform_int_distribution<int> Distribution(1, 10);
  return Distribution(Generator);
}
}  // namespace ExamplePlugin

class ExamplePluginInstance final : public IPlugin {
 public:
  const char* GetName() const override { return "ExamplePlugin"; }

  uint32_t GetApiVersion() const override { return BROCCOLI_PLUGIN_API_VERSION; }

  bool OnLoad(PluginContext& InContext) override {
    Context = InContext;
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

extern "C" __declspec(dllexport) IPlugin* CreateBroccoliPlugin() {
  return new ExamplePluginInstance();
}

extern "C" __declspec(dllexport) void DestroyBroccoliPlugin(IPlugin* Plugin) { delete Plugin; }
