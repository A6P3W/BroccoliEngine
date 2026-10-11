#pragma once
#include <string_view>

#include "BroccoliEngineAPI.h"

namespace BEPlugins {
BROCCOLI_ENGINE_API bool IsAvailable(std::string_view PluginName);
}
