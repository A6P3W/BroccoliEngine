#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

struct FAutomationSystemCommandDescriptor {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema = {
      {"type", "object"}, {"properties", nlohmann::json::object()}, {"additionalProperties", false}
  };
  EAutomationPermission Permission = EAutomationPermission::SystemMutation;
  std::function<nlohmann::json(const nlohmann::json&)> Handler;
};

struct FAutomationSystemCommandSnapshot {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema;
  EAutomationPermission Permission = EAutomationPermission::SystemMutation;
};

class BROCCOLI_ENGINE_API FAutomationSystemCommandRegistry {
 public:
  bool RegisterCommand(
      FAutomationSystemCommandDescriptor Descriptor, std::string* OutError = nullptr
  );

  const FAutomationSystemCommandDescriptor* FindCommand(std::string_view CommandName) const;

  std::vector<FAutomationSystemCommandSnapshot> GetCommands() const;

  void Freeze();
  bool IsFrozen() const;

 private:
  std::unordered_map<std::string, FAutomationSystemCommandDescriptor> Commands;
  bool Frozen = false;
};
