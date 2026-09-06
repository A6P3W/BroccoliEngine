#pragma once

#include <concepts>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "AutomationTypes.h"
#include "UMath.h"

template <class T, class TEnable = void>
struct TAutomationJsonConverter;

template <>
struct TAutomationJsonConverter<bool> {
  static bool FromJson(const nlohmann::json& Json, bool& OutValue) {
    if (!Json.is_boolean()) {
      return false;
    }
    OutValue = Json.get<bool>();
    return true;
  }

  static nlohmann::json ToJson(bool Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "boolean"}}; }
};

template <std::integral T>
  requires(!std::same_as<T, bool>)
struct TAutomationJsonConverter<T> {
  static bool FromJson(const nlohmann::json& Json, T& OutValue) {
    try {
      if (!Json.is_number_integer() && !Json.is_number_unsigned()) {
        return false;
      }
      OutValue = Json.get<T>();
      return true;
    } catch (const nlohmann::json::exception&) {
      return false;
    }
  }

  static nlohmann::json ToJson(T Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "integer"}}; }
};

template <std::floating_point T>
struct TAutomationJsonConverter<T> {
  static bool FromJson(const nlohmann::json& Json, T& OutValue) {
    try {
      if (!Json.is_number()) {
        return false;
      }
      OutValue = Json.get<T>();
      return true;
    } catch (const nlohmann::json::exception&) {
      return false;
    }
  }

  static nlohmann::json ToJson(T Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "number"}}; }
};

template <>
struct TAutomationJsonConverter<std::string> {
  static bool FromJson(const nlohmann::json& Json, std::string& OutValue) {
    if (!Json.is_string()) {
      return false;
    }
    OutValue = Json.get<std::string>();
    return true;
  }

  static nlohmann::json ToJson(const std::string& Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "string"}}; }
};

template <class T>
struct TAutomationJsonConverter<std::vector<T>> {
  static bool FromJson(const nlohmann::json& Json, std::vector<T>& OutValue) {
    if (!Json.is_array()) {
      return false;
    }
    std::vector<T> Values;
    Values.reserve(Json.size());
    for (const nlohmann::json& Element : Json) {
      T Value{};
      if (!TAutomationJsonConverter<T>::FromJson(Element, Value)) {
        return false;
      }
      Values.push_back(std::move(Value));
    }
    OutValue = std::move(Values);
    return true;
  }

  static nlohmann::json ToJson(const std::vector<T>& Value) {
    nlohmann::json Result = nlohmann::json::array();
    for (const T& Element : Value) {
      Result.push_back(TAutomationJsonConverter<T>::ToJson(Element));
    }
    return Result;
  }

  static nlohmann::json GetSchema() {
    return {{"type", "array"}, {"items", TAutomationJsonConverter<T>::GetSchema()}};
  }
};

template <class T>
struct TAutomationJsonConverter<std::optional<T>> {
  static bool FromJson(const nlohmann::json& Json, std::optional<T>& OutValue) {
    if (Json.is_null()) {
      OutValue.reset();
      return true;
    }
    T Value{};
    if (!TAutomationJsonConverter<T>::FromJson(Json, Value)) {
      return false;
    }
    OutValue = std::move(Value);
    return true;
  }

  static nlohmann::json ToJson(const std::optional<T>& Value) {
    return Value ? TAutomationJsonConverter<T>::ToJson(*Value) : nlohmann::json(nullptr);
  }

  static nlohmann::json GetSchema() {
    nlohmann::json Schema = TAutomationJsonConverter<T>::GetSchema();
    Schema["type"] = nlohmann::json::array({Schema["type"], "null"});
    return Schema;
  }
};

template <>
struct TAutomationJsonConverter<FVector2D> {
  static bool FromJson(const nlohmann::json& Json, FVector2D& OutValue) {
    if (!Json.is_object() || !Json.contains("x") || !Json.contains("y")) {
      return false;
    }
    return TAutomationJsonConverter<float>::FromJson(Json["x"], OutValue.X) &&
           TAutomationJsonConverter<float>::FromJson(Json["y"], OutValue.Y);
  }

  static nlohmann::json ToJson(const FVector2D& Value) { return {{"x", Value.X}, {"y", Value.Y}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties",
         {{"x", TAutomationJsonConverter<float>::GetSchema()},
          {"y", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"x", "y"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<FRotator> {
  static bool FromJson(const nlohmann::json& Json, FRotator& OutValue) {
    return Json.is_object() && Json.contains("rotation") &&
           TAutomationJsonConverter<float>::FromJson(Json["rotation"], OutValue.Rotation);
  }

  static nlohmann::json ToJson(const FRotator& Value) { return {{"rotation", Value.Rotation}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties", {{"rotation", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"rotation"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<FScale> {
  static bool FromJson(const nlohmann::json& Json, FScale& OutValue) {
    return Json.is_object() && Json.contains("scale") &&
           TAutomationJsonConverter<float>::FromJson(Json["scale"], OutValue.Scale);
  }

  static nlohmann::json ToJson(const FScale& Value) { return {{"scale", Value.Scale}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties", {{"scale", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"scale"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<nlohmann::json> {
  static nlohmann::json ToJson(const nlohmann::json& Value) { return Value; }
};
