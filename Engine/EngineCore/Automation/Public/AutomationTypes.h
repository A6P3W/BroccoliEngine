#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

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
  ActorNotFound
};

struct FAutomationConfig {
  bool Enabled = false;
  std::string BindAddress = "127.0.0.1";
  uint16_t Port = 39100;
  uint32_t RequestTimeoutSeconds = 3;
  size_t MaxRequestBodyBytes = 1024 * 1024;
};

nlohmann::json MakeAutomationSuccess(nlohmann::json Data = nlohmann::json::object());

nlohmann::json MakeAutomationError(EAutomationErrorCode ErrorCode, std::string_view Message);

std::string_view ToAutomationErrorCodeString(EAutomationErrorCode ErrorCode);
