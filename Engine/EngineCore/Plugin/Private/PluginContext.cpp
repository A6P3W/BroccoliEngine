#include "PluginContext.h"

#include "Log.h"

void PluginContext::LogInfo(const char* Message) const {
  M_LOG(Log, "Plugin: {}", Message != nullptr ? Message : "");
}

void PluginContext::LogWarning(const char* Message) const {
  M_LOG(Warning, "Plugin: {}", Message != nullptr ? Message : "");
}

void PluginContext::LogError(const char* Message) const {
  M_LOG(Error, "Plugin: {}", Message != nullptr ? Message : "");
}

bool PluginContext::RegisterNetworkTransport(
    std::string Name, FNetworkTransportFactory Factory
) const {
  return NetworkTransportRegistry::GetInstance().RegisterOwned(
      std::move(Name), std::move(Factory), ModuleOwner
  );
}
