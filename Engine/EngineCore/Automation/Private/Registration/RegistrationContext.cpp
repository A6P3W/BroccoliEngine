#include <stdexcept>
#include <utility>

#include "Detail/AutomationRegistrationContext.h"
#include "Log.h"
#include "Registration/RegistrationStore.h"
#include "Registry/ActorMethodRegistry.h"
#include "Registry/ComponentMethodRegistry.h"

namespace BroccoliAutomationDetail {
FAutomationRegistrationContext::FAutomationRegistrationContext(
    void* InActorRegistry, void* InComponentRegistry, std::string InModuleOwner
)
    : ActorRegistry(InActorRegistry),
      ComponentRegistry(InComponentRegistry),
      ModuleOwner(std::move(InModuleOwner)) {}

void FAutomationRegistrationContext::RegisterActorMethod(
    std::string ClassName,
    std::string Name,
    std::string Description,
    nlohmann::json InputSchema,
    FAutomationActorHandler Handler
) {
  auto* Registry = static_cast<FAutomationActorMethodRegistry*>(ActorRegistry);
  if (!Registry) {
    throw std::runtime_error("Automation actor registry is unavailable.");
  }
  FAutomationMethodDescriptor Descriptor;
  Descriptor.ModuleOwner = ModuleOwner;
  Descriptor.Name = std::move(Name);
  Descriptor.Description = std::move(Description);
  Descriptor.InputSchema = std::move(InputSchema);
  Descriptor.Handler = std::move(Handler);
  std::string Error;
  if (!Registry->RegisterMethod(std::move(ClassName), std::move(Descriptor), &Error)) {
    M_LOG(Error, "Automation method registration failed: {}", Error);
    throw std::runtime_error(Error);
  }
}

void FAutomationRegistrationContext::RegisterComponentMethod(
    std::string ClassName,
    std::string Name,
    std::string Description,
    nlohmann::json InputSchema,
    FAutomationComponentHandler Handler
) {
  auto* Registry = static_cast<FAutomationComponentMethodRegistry*>(ComponentRegistry);
  if (!Registry) {
    throw std::runtime_error("Automation component registry is unavailable.");
  }
  FAutomationComponentMethodDescriptor Descriptor;
  Descriptor.ModuleOwner = ModuleOwner;
  Descriptor.Name = std::move(Name);
  Descriptor.Description = std::move(Description);
  Descriptor.InputSchema = std::move(InputSchema);
  Descriptor.Handler = std::move(Handler);
  std::string Error;
  if (!Registry->RegisterMethod(std::move(ClassName), std::move(Descriptor), &Error)) {
    M_LOG(Error, "Automation component method registration failed: {}", Error);
    throw std::runtime_error(Error);
  }
}

FAutomationRegistrationToken::FAutomationRegistrationToken(
    FAutomationRegistrationCallback Callback
) {
  FAutomationRegistrationStore::Get().AddCallback(Callback);
}

void RegisterCallbackOwned(FAutomationRegistrationCallback Callback, std::string ModuleOwner) {
  FAutomationRegistrationStore::Get().AddCallback(Callback, std::move(ModuleOwner));
}

void UnregisterControlModule(std::string_view ModuleOwner) {
  FAutomationRegistrationStore::Get().UnregisterModule(ModuleOwner);
}
}  // namespace BroccoliAutomationDetail
