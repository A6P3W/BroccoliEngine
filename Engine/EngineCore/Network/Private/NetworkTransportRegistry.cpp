#include "NetworkTransportRegistry.h"

#include <utility>

std::unique_ptr<INetworkTransport> CreateENetTransport();

NetworkTransportRegistry::NetworkTransportRegistry() {
  RegisterOwned("ENet", CreateENetTransport, "Engine:Network");
}

NetworkTransportRegistry& NetworkTransportRegistry::GetInstance() {
  static NetworkTransportRegistry Instance;
  return Instance;
}

bool NetworkTransportRegistry::RegisterOwned(
    std::string Name, FNetworkTransportFactory Factory, std::string ModuleOwner
) {
  if (Name.empty() || ModuleOwner.empty() || !Factory || IsRegistered(Name)) return false;
  Registrations.emplace(std::move(Name), FRegistration{std::move(Factory), std::move(ModuleOwner)});
  return true;
}

bool NetworkTransportRegistry::IsRegistered(std::string_view Name) const {
  return Registrations.contains(Name);
}

std::unique_ptr<INetworkTransport> NetworkTransportRegistry::Create(std::string_view Name) {
  const auto Iterator = Registrations.find(Name);
  return Iterator == Registrations.end() ? nullptr : Iterator->second.Factory();
}

void NetworkTransportRegistry::UnregisterModule(std::string_view ModuleOwner) {
  std::erase_if(Registrations, [ModuleOwner](const auto& Entry) {
    return Entry.second.ModuleOwner == ModuleOwner;
  });
}

std::vector<std::string> NetworkTransportRegistry::GetRegisteredNames() const {
  std::vector<std::string> Names;
  for (const auto& [Name, Registration] : Registrations) Names.push_back(Name);
  return Names;
}
