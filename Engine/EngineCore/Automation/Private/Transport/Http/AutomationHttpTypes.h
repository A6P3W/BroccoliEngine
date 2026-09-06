#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "AutomationTypes.h"

struct FAutomationHttpResponse {
  int StatusCode = 500;
  nlohmann::json Body =
      MakeAutomationError(EAutomationErrorCode::InternalError, "The automation request failed.");
};

struct FAutomationActorQueryText {
  std::optional<std::string> ClassName;
  std::optional<std::string> InstanceName;
  bool bHasUnknownParameter = false;
  bool bHasDuplicateParameter = false;
};

struct FAutomationLogQueryText {
  std::optional<std::string> Limit;
  std::optional<std::string> Level;
  std::optional<std::string> AfterSequence;
  bool bHasUnknownParameter = false;
  bool bHasDuplicateParameter = false;
};
