#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AutomationTypes.h"

class MActorComponent;

struct FAutomationComponentMethodDescriptor {
  std::string ModuleOwner = "Static";
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema = {
      {"type", "object"}, {"properties", nlohmann::json::object()}, {"additionalProperties", false}
  };
  std::function<nlohmann::json(MActorComponent&, const nlohmann::json&)> Handler;
};

struct FAutomationComponentMethodSnapshot {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema;
};

class FAutomationComponentMethodRegistry {
 public:
  bool RegisterMethod(
      std::string ClassName,
      FAutomationComponentMethodDescriptor Descriptor,
      std::string* OutError = nullptr
  );

  const FAutomationComponentMethodDescriptor* FindMethod(
      std::string_view ClassName, std::string_view MethodName
  ) const;

  std::vector<FAutomationComponentMethodSnapshot> GetMethodsForClass(
      std::string_view ClassName
  ) const;

  void UnregisterModule(std::string_view ModuleOwner);

 private:
  using FMethodMap = std::unordered_map<std::string, FAutomationComponentMethodDescriptor>;

  std::unordered_map<std::string, FMethodMap> MethodsByClass;
};
