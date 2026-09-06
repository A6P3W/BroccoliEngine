#pragma once

#include <cstdint>

class PluginContext;

class IPlugin {
 public:
  virtual ~IPlugin() = default;

  virtual const char* GetName() const = 0;
  virtual uint32_t GetApiVersion() const = 0;

  virtual bool OnLoad(PluginContext& Context) = 0;
  virtual void OnUnload() = 0;
  virtual void OnUpdate(float DeltaTime) {}
};

using FCreateBroccoliPluginFunction = IPlugin* (*)();
using FDestroyBroccoliPluginFunction = void (*)(IPlugin* Plugin);
