#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

class AActor;

struct FAutomationMethodDescriptor {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema = {
      {"type", "object"}, {"properties", nlohmann::json::object()}, {"additionalProperties", false}
  };
  EAutomationPermission Permission = EAutomationPermission::ReadOnly;
  std::function<nlohmann::json(AActor&, const nlohmann::json&)> Handler;
};

struct FAutomationMethodSnapshot {
  std::string Name;
  std::string Description;
  nlohmann::json InputSchema;
  EAutomationPermission Permission = EAutomationPermission::ReadOnly;
};

class BROCCOLI_ENGINE_API FAutomationMethodRegistry {
 public:
  bool RegisterMethod(
      std::string ClassName, FAutomationMethodDescriptor Descriptor, std::string* OutError = nullptr
  );

  const FAutomationMethodDescriptor* FindMethod(
      std::string_view ClassName, std::string_view MethodName
  ) const;

  std::vector<FAutomationMethodSnapshot> GetMethodsForClass(std::string_view ClassName) const;

  void Freeze();
  bool IsFrozen() const;

 private:
  using FMethodMap = std::unordered_map<std::string, FAutomationMethodDescriptor>;

  std::unordered_map<std::string, FMethodMap> MethodsByClass;
  bool Frozen = false;
};

using FAutomationMethodRegistrationCallback = void (*)(FAutomationMethodRegistry&);
