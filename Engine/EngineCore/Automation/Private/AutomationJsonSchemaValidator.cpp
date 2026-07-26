#include "AutomationJsonSchemaValidator.h"

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace {
constexpr std::string_view RootPath = "$";

bool Fail(FAutomationSchemaValidationError& OutError, std::string Path, std::string Message) {
  OutError.JsonPath = std::move(Path);
  OutError.Message = std::move(Message);
  return false;
}

bool IsNonNegativeInteger(const nlohmann::json& Value) {
  if (Value.is_number_unsigned()) {
    return Value.get<uint64_t>() <= static_cast<uint64_t>((std::numeric_limits<size_t>::max)());
  }
  if (!Value.is_number_integer()) {
    return false;
  }
  const int64_t Number = Value.get<int64_t>();
  return Number >= 0 && static_cast<uint64_t>(Number) <=
                            static_cast<uint64_t>((std::numeric_limits<size_t>::max)());
}

bool IsFiniteNumber(const nlohmann::json& Value) {
  return Value.is_number() && std::isfinite(Value.get<double>());
}

bool IsSupportedType(std::string_view Type) {
  return Type == "object" || Type == "array" || Type == "string" || Type == "number" ||
         Type == "integer" || Type == "boolean" || Type == "null";
}

bool IsAllowedKeyword(std::string_view Keyword) {
  return Keyword == "type" || Keyword == "properties" || Keyword == "required" ||
         Keyword == "additionalProperties" || Keyword == "items" || Keyword == "enum" ||
         Keyword == "minimum" || Keyword == "maximum" || Keyword == "minLength" ||
         Keyword == "maxLength" || Keyword == "minItems" || Keyword == "maxItems";
}

std::string ChildPath(std::string_view Parent, std::string_view Property) {
  std::string Result(Parent);
  Result.push_back('.');
  Result.append(Property);
  return Result;
}

std::string ArrayPath(std::string_view Parent, size_t Index) {
  return std::string(Parent) + "[" + std::to_string(Index) + "]";
}

bool MatchesType(const nlohmann::json& Value, std::string_view Type);

bool ValidateSchemaNode(
    const nlohmann::json& Schema, std::string Path, FAutomationSchemaValidationError& OutError
) {
  if (!Schema.is_object()) {
    return Fail(OutError, std::move(Path), "schema must be an object.");
  }

  for (const auto& [Keyword, KeywordValue] : Schema.items()) {
    (void)KeywordValue;
    if (!IsAllowedKeyword(Keyword)) {
      return Fail(OutError, ChildPath(Path, Keyword), "unsupported schema keyword.");
    }
  }

  if (!Schema.contains("type") || !Schema["type"].is_string()) {
    return Fail(OutError, ChildPath(Path, "type"), "type must be a string.");
  }
  const std::string Type = Schema["type"].get<std::string>();
  if (!IsSupportedType(Type)) {
    return Fail(OutError, ChildPath(Path, "type"), "unsupported schema type.");
  }

  const bool HasObjectKeyword = Schema.contains("properties") || Schema.contains("required") ||
                                Schema.contains("additionalProperties");
  if (HasObjectKeyword && Type != "object") {
    return Fail(OutError, std::move(Path), "object schema keywords require type object.");
  }
  const bool HasArrayKeyword =
      Schema.contains("items") || Schema.contains("minItems") || Schema.contains("maxItems");
  if (HasArrayKeyword && Type != "array") {
    return Fail(OutError, std::move(Path), "array schema keywords require type array.");
  }
  const bool HasStringKeyword = Schema.contains("minLength") || Schema.contains("maxLength");
  if (HasStringKeyword && Type != "string") {
    return Fail(OutError, std::move(Path), "string schema keywords require type string.");
  }
  const bool HasNumberKeyword = Schema.contains("minimum") || Schema.contains("maximum");
  if (HasNumberKeyword && Type != "number" && Type != "integer") {
    return Fail(
        OutError, std::move(Path), "numeric schema keywords require type number or integer."
    );
  }

  if (Schema.contains("properties")) {
    const nlohmann::json& Properties = Schema["properties"];
    if (!Properties.is_object()) {
      return Fail(OutError, ChildPath(Path, "properties"), "properties must be an object.");
    }
    for (const auto& [PropertyName, PropertySchema] : Properties.items()) {
      if (!ValidateSchemaNode(
              PropertySchema, ChildPath(ChildPath(Path, "properties"), PropertyName), OutError
          )) {
        return false;
      }
    }
  }

  if (Schema.contains("required")) {
    const nlohmann::json& Required = Schema["required"];
    if (!Required.is_array()) {
      return Fail(OutError, ChildPath(Path, "required"), "required must be an array.");
    }
    if (!Schema.contains("properties")) {
      return Fail(OutError, ChildPath(Path, "required"), "required needs a properties definition.");
    }
    std::unordered_set<std::string> RequiredNames;
    for (size_t Index = 0; Index < Required.size(); ++Index) {
      if (!Required[Index].is_string()) {
        return Fail(
            OutError,
            ArrayPath(ChildPath(Path, "required"), Index),
            "required entries must be strings."
        );
      }
      const std::string Name = Required[Index].get<std::string>();
      if (!Schema["properties"].contains(Name)) {
        return Fail(
            OutError,
            ArrayPath(ChildPath(Path, "required"), Index),
            "required property is not defined in properties."
        );
      }
      if (!RequiredNames.insert(Name).second) {
        return Fail(
            OutError,
            ArrayPath(ChildPath(Path, "required"), Index),
            "required entries must be unique."
        );
      }
    }
  }

  if (Schema.contains("additionalProperties") && !Schema["additionalProperties"].is_boolean()) {
    return Fail(
        OutError, ChildPath(Path, "additionalProperties"), "additionalProperties must be a boolean."
    );
  }

  if (Schema.contains("items") &&
      !ValidateSchemaNode(Schema["items"], ChildPath(Path, "items"), OutError)) {
    return false;
  }

  if (Schema.contains("enum")) {
    const nlohmann::json& EnumValues = Schema["enum"];
    if (!EnumValues.is_array() || EnumValues.empty()) {
      return Fail(OutError, ChildPath(Path, "enum"), "enum must be a non-empty array.");
    }
    for (size_t Index = 0; Index < EnumValues.size(); ++Index) {
      if (!MatchesType(EnumValues[Index], Type)) {
        return Fail(
            OutError,
            ArrayPath(ChildPath(Path, "enum"), Index),
            "enum value does not match the schema type."
        );
      }
    }
  }

  if (Schema.contains("minimum") && !IsFiniteNumber(Schema["minimum"])) {
    return Fail(OutError, ChildPath(Path, "minimum"), "minimum must be a finite number.");
  }
  if (Schema.contains("maximum") && !IsFiniteNumber(Schema["maximum"])) {
    return Fail(OutError, ChildPath(Path, "maximum"), "maximum must be a finite number.");
  }
  if (Schema.contains("minimum") && Schema.contains("maximum") &&
      Schema["minimum"].get<double>() > Schema["maximum"].get<double>()) {
    return Fail(OutError, std::move(Path), "minimum must not exceed maximum.");
  }

  for (const std::string_view Keyword : {"minLength", "maxLength", "minItems", "maxItems"}) {
    const std::string KeywordText(Keyword);
    if (Schema.contains(KeywordText) && !IsNonNegativeInteger(Schema[KeywordText])) {
      return Fail(
          OutError, ChildPath(Path, Keyword), KeywordText + " must be a non-negative integer."
      );
    }
  }

  if (Schema.contains("minLength") && Schema.contains("maxLength") &&
      Schema["minLength"].get<uint64_t>() > Schema["maxLength"].get<uint64_t>()) {
    return Fail(OutError, std::move(Path), "minLength must not exceed maxLength.");
  }
  if (Schema.contains("minItems") && Schema.contains("maxItems") &&
      Schema["minItems"].get<uint64_t>() > Schema["maxItems"].get<uint64_t>()) {
    return Fail(OutError, std::move(Path), "minItems must not exceed maxItems.");
  }
  return true;
}

bool MatchesType(const nlohmann::json& Value, std::string_view Type) {
  if (Type == "object") {
    return Value.is_object();
  }
  if (Type == "array") {
    return Value.is_array();
  }
  if (Type == "string") {
    return Value.is_string();
  }
  if (Type == "number") {
    return IsFiniteNumber(Value);
  }
  if (Type == "integer") {
    return Value.is_number_integer() || Value.is_number_unsigned();
  }
  if (Type == "boolean") {
    return Value.is_boolean();
  }
  return Value.is_null();
}

bool ValidateValueNode(
    const nlohmann::json& Schema,
    const nlohmann::json& Value,
    std::string Path,
    FAutomationSchemaValidationError& OutError
) {
  const std::string Type = Schema["type"].get<std::string>();
  if (!MatchesType(Value, Type)) {
    return Fail(OutError, std::move(Path), "expected " + Type + ".");
  }

  if (Schema.contains("enum")) {
    bool Found = false;
    for (const nlohmann::json& Candidate : Schema["enum"]) {
      if (Candidate == Value) {
        Found = true;
        break;
      }
    }
    if (!Found) {
      return Fail(OutError, std::move(Path), "value is not in enum.");
    }
  }

  if (Type == "object") {
    if (Schema.contains("required")) {
      for (const nlohmann::json& RequiredNameValue : Schema["required"]) {
        const std::string RequiredName = RequiredNameValue.get<std::string>();
        if (!Value.contains(RequiredName)) {
          return Fail(OutError, ChildPath(Path, RequiredName), "required property is missing.");
        }
      }
    }

    const nlohmann::json EmptyProperties = nlohmann::json::object();
    const nlohmann::json& Properties =
        Schema.contains("properties") ? Schema["properties"] : EmptyProperties;
    const bool AllowAdditional =
        !Schema.contains("additionalProperties") || Schema["additionalProperties"].get<bool>();
    for (const auto& [PropertyName, PropertyValue] : Value.items()) {
      if (!Properties.contains(PropertyName)) {
        if (!AllowAdditional) {
          return Fail(
              OutError, ChildPath(Path, PropertyName), "additional property is not allowed."
          );
        }
        continue;
      }
      if (!ValidateValueNode(
              Properties[PropertyName], PropertyValue, ChildPath(Path, PropertyName), OutError
          )) {
        return false;
      }
    }
  } else if (Type == "array") {
    const size_t Count = Value.size();
    if (Schema.contains("minItems") && Count < Schema["minItems"].get<size_t>()) {
      return Fail(OutError, std::move(Path), "array has too few items.");
    }
    if (Schema.contains("maxItems") && Count > Schema["maxItems"].get<size_t>()) {
      return Fail(OutError, std::move(Path), "array has too many items.");
    }
    if (Schema.contains("items")) {
      for (size_t Index = 0; Index < Count; ++Index) {
        if (!ValidateValueNode(Schema["items"], Value[Index], ArrayPath(Path, Index), OutError)) {
          return false;
        }
      }
    }
  } else if (Type == "string") {
    const size_t Length = Value.get_ref<const std::string&>().size();
    if (Schema.contains("minLength") && Length < Schema["minLength"].get<size_t>()) {
      return Fail(OutError, std::move(Path), "string is shorter than minLength.");
    }
    if (Schema.contains("maxLength") && Length > Schema["maxLength"].get<size_t>()) {
      return Fail(OutError, std::move(Path), "string is longer than maxLength.");
    }
  } else if (Type == "number" || Type == "integer") {
    const double Number = Value.get<double>();
    if (!std::isfinite(Number)) {
      return Fail(OutError, std::move(Path), "number must be finite.");
    }
    if (Schema.contains("minimum") && Number < Schema["minimum"].get<double>()) {
      return Fail(OutError, std::move(Path), "number is below minimum.");
    }
    if (Schema.contains("maximum") && Number > Schema["maximum"].get<double>()) {
      return Fail(OutError, std::move(Path), "number is above maximum.");
    }
  }

  return true;
}
}  // namespace

bool FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
    const nlohmann::json& Schema, FAutomationSchemaValidationError& OutError
) {
  OutError = {};
  return ValidateSchemaNode(Schema, std::string(RootPath), OutError);
}

bool FAutomationJsonSchemaValidator::ValidateValue(
    const nlohmann::json& Schema,
    const nlohmann::json& Value,
    FAutomationSchemaValidationError& OutError
) {
  OutError = {};
  if (!ValidateSchemaNode(Schema, std::string(RootPath), OutError)) {
    return false;
  }
  return ValidateValueNode(Schema, Value, std::string(RootPath), OutError);
}
