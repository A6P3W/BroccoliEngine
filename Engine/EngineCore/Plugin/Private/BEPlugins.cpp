#include "BEPlugins.h"

#include "PluginHost.h"

bool BEPlugins::IsAvailable(std::string_view PluginName) {
  return PluginHost::GetInstance().IsPluginActive(PluginName);
}
