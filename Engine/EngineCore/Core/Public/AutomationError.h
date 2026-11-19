#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string_view>

/// @brief Automationエラーコードの列挙型
enum class EAutomationErrorCode : uint8_t {
  None = 0,
  InvalidRequest,
  InvalidJson,
  InvalidArgument,
  RequestTooLarge,
  WorldNotAvailable,
  RequestTimeout,
  EngineShuttingDown,
  InternalError,
  ActorNotFound,
  ClassNotRegistered,
  ActorPendingDestroy,
  ComponentNotFound,
  ComponentPendingDestroy,
  Conflict,
  MethodNotRegistered,
  CommandNotRegistered
};

nlohmann::json MakeAutomationError(EAutomationErrorCode ErrorCode, std::string_view Message);

std::string_view ToAutomationErrorCodeString(EAutomationErrorCode ErrorCode);
