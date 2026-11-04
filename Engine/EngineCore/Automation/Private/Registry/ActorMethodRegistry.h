#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AutomationTypes.h"

class AActor;

struct FAutomationMethodDescriptor {
  std::string ModuleOwner = "Static";
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema = {
      {"type", "object"}, {"properties", nlohmann::json::object()}, {"additionalProperties", false}
  };
  std::function<nlohmann::json(AActor&, const nlohmann::json&)> Handler;
};
struct FAutomationMethodSnapshot {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema;
};

class FAutomationActorMethodRegistry {
 public:
  bool RegisterMethod(
      std::string ClassName, FAutomationMethodDescriptor Descriptor, std::string* OutError = nullptr
  );

  const FAutomationMethodDescriptor* FindMethod(
      std::string_view ClassName, std::string_view MethodName
  ) const;

  std::vector<FAutomationMethodSnapshot> GetMethodsForClass(std::string_view ClassName) const;

  void UnregisterModule(std::string_view ModuleOwner);

 private:
  using FMethodMap = std::unordered_map<std::string, FAutomationMethodDescriptor>;

  std::unordered_map<std::string, FMethodMap> MethodsByClass;
};
