#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "BroccoliEngineAPI.h"

struct FAutomationSchemaValidationError {
  std::string JsonPath;
  std::string Message;
};

class BROCCOLI_ENGINE_API FAutomationJsonSchemaValidator {
 public:
  static bool ValidateSchemaDefinition(
      const nlohmann::json& Schema, FAutomationSchemaValidationError& OutError
  );

  static bool ValidateValue(
      const nlohmann::json& Schema,
      const nlohmann::json& Value,
      FAutomationSchemaValidationError& OutError
  );
};
