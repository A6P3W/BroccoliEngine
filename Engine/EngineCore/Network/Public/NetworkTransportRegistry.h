#pragma once

#include <map>
#include <string_view>

#include "NetworkTransport.h"

using FNetworkTransportFactory = std::function<std::unique_ptr<INetworkTransport>()>;

class BROCCOLI_ENGINE_API NetworkTransportRegistry {
 public:
  static NetworkTransportRegistry& GetInstance();
  bool RegisterOwned(std::string Name, FNetworkTransportFactory Factory, std::string ModuleOwner);
  bool IsRegistered(std::string_view Name) const;
  std::unique_ptr<INetworkTransport> Create(std::string_view Name);
  void UnregisterModule(std::string_view ModuleOwner);
  std::vector<std::string> GetRegisteredNames() const;

 private:
  NetworkTransportRegistry();
  struct FRegistration {
    FNetworkTransportFactory Factory;
    std::string ModuleOwner;
  };
  std::map<std::string, FRegistration, std::less<>> Registrations;
};
