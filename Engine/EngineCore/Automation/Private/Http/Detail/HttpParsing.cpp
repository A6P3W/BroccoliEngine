#include "HttpParsing.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <initializer_list>
#include <limits>
namespace AutomationHttpDetail {
bool HasOnlyAllowedFields(
    const nlohmann::json& Object, std::initializer_list<std::string_view> AllowedFields
) {
  if (!Object.is_object()) {
    return false;
  }
  for (const auto& [FieldName, Value] : Object.items()) {
    (void)Value;
    bool Allowed = false;
    for (const std::string_view AllowedField : AllowedFields) {
      if (FieldName == AllowedField) {
        Allowed = true;
        break;
      }
    }
    if (!Allowed) {
      return false;
    }
  }
  return true;
}

bool TryReadFiniteFloat(
    const nlohmann::json& Object, std::string_view FieldName, float& OutValue, std::string& OutError
) {
  const std::string FieldNameString(FieldName);
  if (!Object.contains(FieldNameString) || !Object[FieldNameString].is_number()) {
    OutError = FieldNameString + " must be a number.";
    return false;
  }

  const double Value = Object[FieldNameString].get<double>();
  if (!std::isfinite(Value) ||
      Value < static_cast<double>((std::numeric_limits<float>::lowest)()) ||
      Value > static_cast<double>((std::numeric_limits<float>::max)())) {
    OutError = FieldNameString + " must be a finite 32-bit floating-point value.";
    return false;
  }

  OutValue = static_cast<float>(Value);
  return true;
}

bool TryParseUnsigned(std::string_view Text, uint64_t& OutValue) {
  OutValue = 0;
  if (Text.empty()) {
    return false;
  }
  const char* Begin = Text.data();
  const char* End = Begin + Text.size();
  const auto [Position, Error] = std::from_chars(Begin, End, OutValue, 10);
  return Error == std::errc() && Position == End;
}

std::optional<ELogLevel> ParseLogLevel(std::string_view Text) {
  std::string Lowercase(Text);
  std::transform(
      Lowercase.begin(), Lowercase.end(), Lowercase.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
      }
  );
  if (Lowercase == "debug") {
    return ELogLevel::Debug;
  }
  if (Lowercase == "log" || Lowercase == "info") {
    return ELogLevel::Log;
  }
  if (Lowercase == "warning") {
    return ELogLevel::Warning;
  }
  if (Lowercase == "error") {
    return ELogLevel::Error;
  }
  return std::nullopt;
}

bool TryParseActorId(std::string_view Text, FActorId& OutActorId) {
  OutActorId = InvalidActorId;
  if (Text.empty()) {
    return false;
  }

  FActorId ParsedActorId = InvalidActorId;
  const char* Begin = Text.data();
  const char* End = Begin + Text.size();
  const auto [Position, Error] = std::from_chars(Begin, End, ParsedActorId, 10);
  if (Error != std::errc() || Position != End || ParsedActorId == InvalidActorId) {
    return false;
  }

  OutActorId = ParsedActorId;
  return true;
}

bool TryParseComponentId(std::string_view Text, FComponentId& OutComponentId) {
  OutComponentId = InvalidComponentId;
  uint64_t ParsedComponentId = 0;
  if (!TryParseUnsigned(Text, ParsedComponentId) || ParsedComponentId == 0) {
    return false;
  }
  OutComponentId = ParsedComponentId;
  return true;
}

namespace {
bool TryParseLocationObject(
    const nlohmann::json& LocationJson, FOptionalVector3D& OutLocation, std::string& OutError
) {
  if (!HasOnlyAllowedFields(LocationJson, {"x", "y", "z"})) {
    OutError = "location contains an unknown field or is not an object.";
    return false;
  }
  if (LocationJson.contains("x")) {
    float Val = 0.0f;
    if (!TryReadFiniteFloat(LocationJson, "x", Val, OutError)) return false;
    OutLocation.X = Val;
  }
  if (LocationJson.contains("y")) {
    float Val = 0.0f;
    if (!TryReadFiniteFloat(LocationJson, "y", Val, OutError)) return false;
    OutLocation.Y = Val;
  }
  if (LocationJson.contains("z")) {
    float Val = 0.0f;
    if (!TryReadFiniteFloat(LocationJson, "z", Val, OutError)) return false;
    OutLocation.Z = Val;
  }
  return true;
}

bool TryParseRotationJson(
    const nlohmann::json& RotationJson, FOptionalRotator3D& OutRotation, std::string& OutError
) {
  if (RotationJson.is_number()) {
    float Roll = 0.0f;
    const double Value = RotationJson.get<double>();
    if (!std::isfinite(Value) ||
        Value < static_cast<double>((std::numeric_limits<float>::lowest)()) ||
        Value > static_cast<double>((std::numeric_limits<float>::max)())) {
      OutError = "rotation must be a finite 32-bit floating-point value.";
      return false;
    }
    OutRotation.Roll = static_cast<float>(Value);
    return true;
  }
  if (RotationJson.is_object()) {
    if (!HasOnlyAllowedFields(RotationJson, {"pitch", "yaw", "roll", "x", "y", "z"})) {
      OutError = "rotation contains an unknown field or is not an object.";
      return false;
    }
    if (RotationJson.contains("pitch")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "pitch", Val, OutError)) return false;
      OutRotation.Pitch = Val;
    } else if (RotationJson.contains("x")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "x", Val, OutError)) return false;
      OutRotation.Pitch = Val;
    }
    if (RotationJson.contains("yaw")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "yaw", Val, OutError)) return false;
      OutRotation.Yaw = Val;
    } else if (RotationJson.contains("y")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "y", Val, OutError)) return false;
      OutRotation.Yaw = Val;
    }
    if (RotationJson.contains("roll")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "roll", Val, OutError)) return false;
      OutRotation.Roll = Val;
    } else if (RotationJson.contains("z")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(RotationJson, "z", Val, OutError)) return false;
      OutRotation.Roll = Val;
    }
    return true;
  }
  OutError = "rotation must be a number or an object.";
  return false;
}

bool TryParseScaleJson(
    const nlohmann::json& ScaleJson, FOptionalScale3D& OutScale, std::string& OutError
) {
  if (ScaleJson.is_number()) {
    float ScaleValue = 1.0f;
    const double Value = ScaleJson.get<double>();
    if (!std::isfinite(Value) ||
        Value < static_cast<double>((std::numeric_limits<float>::lowest)()) ||
        Value > static_cast<double>((std::numeric_limits<float>::max)())) {
      OutError = "scale must be a finite 32-bit floating-point value.";
      return false;
    }
    ScaleValue = static_cast<float>(Value);
    if (ScaleValue <= 0.0f) {
      OutError = "scale must be greater than zero.";
      return false;
    }
    OutScale.X = ScaleValue;
    OutScale.Y = ScaleValue;
    OutScale.Z = ScaleValue;
    return true;
  }
  if (ScaleJson.is_object()) {
    if (!HasOnlyAllowedFields(ScaleJson, {"x", "y", "z"})) {
      OutError = "scale contains an unknown field or is not an object.";
      return false;
    }
    if (ScaleJson.contains("x")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(ScaleJson, "x", Val, OutError)) return false;
      if (Val <= 0.0f) {
        OutError = "scale x must be greater than zero.";
        return false;
      }
      OutScale.X = Val;
    }
    if (ScaleJson.contains("y")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(ScaleJson, "y", Val, OutError)) return false;
      if (Val <= 0.0f) {
        OutError = "scale y must be greater than zero.";
        return false;
      }
      OutScale.Y = Val;
    }
    if (ScaleJson.contains("z")) {
      float Val = 0.0f;
      if (!TryReadFiniteFloat(ScaleJson, "z", Val, OutError)) return false;
      if (Val <= 0.0f) {
        OutError = "scale z must be greater than zero.";
        return false;
      }
      OutScale.Z = Val;
    }
    return true;
  }
  OutError = "scale must be a number or an object.";
  return false;
}
}  // namespace

bool TryParseSpawnRequest(
    const nlohmann::json& Body, FAutomationSpawnActorRequest& OutRequest, std::string& OutError
) {
  if (!HasOnlyAllowedFields(Body, {"className", "transform", "instanceName"})) {
    OutError = "The spawn request contains an unknown field or is not an object.";
    return false;
  }
  if (!Body.contains("className") || !Body["className"].is_string()) {
    OutError = "className must be a string.";
    return false;
  }

  FAutomationSpawnActorRequest Request;
  Request.ClassName = Body["className"].get<std::string>();
  if (Request.ClassName.empty() || Request.ClassName.size() > 128) {
    OutError = "className must contain between 1 and 128 UTF-8 bytes.";
    return false;
  }

  if (Body.contains("instanceName")) {
    if (!Body["instanceName"].is_string()) {
      OutError = "instanceName must be a string.";
      return false;
    }
    std::string InstanceName = Body["instanceName"].get<std::string>();
    if (InstanceName.empty() || InstanceName.size() > 128) {
      OutError = "instanceName must contain between 1 and 128 UTF-8 bytes.";
      return false;
    }
    Request.InstanceName = std::move(InstanceName);
  }

  if (Body.contains("transform")) {
    const nlohmann::json& Transform = Body["transform"];
    if (!HasOnlyAllowedFields(Transform, {"location", "rotation", "scale"})) {
      OutError = "transform contains an unknown field or is not an object.";
      return false;
    }

    if (Transform.contains("location")) {
      FOptionalVector3D OptLocation;
      if (!TryParseLocationObject(Transform["location"], OptLocation, OutError)) {
        return false;
      }
      if (OptLocation.X) Request.Location.X = *OptLocation.X;
      if (OptLocation.Y) Request.Location.Y = *OptLocation.Y;
      if (OptLocation.Z) Request.Location.Z = *OptLocation.Z;
    }

    if (Transform.contains("rotation")) {
      FOptionalRotator3D OptRotation;
      if (!TryParseRotationJson(Transform["rotation"], OptRotation, OutError)) {
        return false;
      }
      if (OptRotation.Pitch) Request.Rotation.Pitch = *OptRotation.Pitch;
      if (OptRotation.Yaw) Request.Rotation.Yaw = *OptRotation.Yaw;
      if (OptRotation.Roll) Request.Rotation.Roll = *OptRotation.Roll;
    }

    if (Transform.contains("scale")) {
      FOptionalScale3D OptScale;
      if (!TryParseScaleJson(Transform["scale"], OptScale, OutError)) {
        return false;
      }
      if (OptScale.X) Request.Scale.X = *OptScale.X;
      if (OptScale.Y) Request.Scale.Y = *OptScale.Y;
      if (OptScale.Z) Request.Scale.Z = *OptScale.Z;
    }
  }

  OutRequest = std::move(Request);
  return true;
}

bool TryParseTransformPatch(
    const nlohmann::json& Body, FAutomationTransformPatch& OutPatch, std::string& OutError
) {
  if (!HasOnlyAllowedFields(Body, {"location", "rotation", "scale"})) {
    OutError = "The transform patch contains an unknown field or is not an object.";
    return false;
  }

  FAutomationTransformPatch Patch;
  if (Body.contains("location")) {
    if (!TryParseLocationObject(Body["location"], Patch.Location, OutError)) {
      return false;
    }
    if (!Patch.Location.HasAnyValue()) {
      OutError = "location must contain at least one of x, y, or z.";
      return false;
    }
  }

  if (Body.contains("rotation")) {
    if (!TryParseRotationJson(Body["rotation"], Patch.Rotation, OutError)) {
      return false;
    }
    if (!Patch.Rotation.HasAnyValue()) {
      OutError = "rotation must contain at least one rotation field.";
      return false;
    }
  }

  if (Body.contains("scale")) {
    if (!TryParseScaleJson(Body["scale"], Patch.Scale, OutError)) {
      return false;
    }
    if (!Patch.Scale.HasAnyValue()) {
      OutError = "scale must contain at least one of x, y, or z.";
      return false;
    }
  }

  if (!Patch.HasAnyValue()) {
    OutError = "At least one transform value must be provided.";
    return false;
  }

  OutPatch = std::move(Patch);
  return true;
}

}  // namespace AutomationHttpDetail
