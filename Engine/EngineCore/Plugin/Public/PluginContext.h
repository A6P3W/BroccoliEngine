#pragma once

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API PluginContext {
 public:
  void LogInfo(const char* Message) const;
  void LogWarning(const char* Message) const;
  void LogError(const char* Message) const;
};
