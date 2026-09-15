#pragma once
#include <nlohmann/json.hpp>
#include <string_view>
#include "AutomationTypes.h"
#include "World/WorldTypes.h"
namespace AutomationHttpDetail {
inline constexpr std::string_view InternalErrorMessage = "The automation request failed.";
inline constexpr std::string_view QueueUnavailableMessage =
    "The automation command queue is not accepting requests.";
inline constexpr std::string_view RequestTimeoutMessage =
    "The main thread did not complete the request before the timeout.";
inline constexpr std::string_view InvalidResponseMessage =
    "The automation command returned an invalid response.";
inline constexpr std::string_view UnknownExceptionMessage =
    "The automation request failed with an unknown exception.";
inline constexpr std::string_view WorldNotAvailableMessage = "No world is currently available.";
inline constexpr std::string_view ActorNotFoundMessage = "The requested actor was not found.";
inline constexpr std::string_view ClassNotRegisteredMessage = "The requested actor class is not registered.";
inline constexpr std::string_view ActorPendingDestroyMessage = "The requested actor is pending destruction.";
inline constexpr std::string_view InvalidActorIdMessage =
    "The actorId must be an unsigned decimal integer greater than zero.";
inline constexpr std::string_view InvalidMethodNameMessage =
    "The methodName must match ^[a-z][a-z0-9_]{0,127}$.";
inline constexpr std::string_view MethodNotRegisteredMessage =
    "The requested method is not registered for this actor class.";
inline constexpr std::string_view PermissionDeniedMessage =
    "The requested method permission is not allowed.";
inline constexpr std::string_view InvalidCommandNameMessage =
    "The commandName must match ^[a-z][a-z0-9_]{0,127}$.";
inline constexpr std::string_view CommandNotRegisteredMessage =
    "The requested system command is not registered.";
inline constexpr std::string_view CommandPermissionDeniedMessage =
    "The requested command permission is not allowed.";
nlohmann::json MakeWorldReadError(EAutomationWorldReadStatus);
nlohmann::json MakeWorldMutationError(EAutomationWorldMutationStatus);
nlohmann::json MakeActorResolveError(EAutomationActorResolveStatus);
nlohmann::json MakeComponentResolveError(EAutomationComponentResolveStatus);
}
