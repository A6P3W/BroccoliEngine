#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include "AutomationTypes.h"
#include "Log.h"
#include "World/WorldTypes.h"
namespace AutomationHttpDetail {
bool IsActorMethodPermissionAllowed(EAutomationPermission);
bool HasOnlyAllowedFields(const nlohmann::json&, std::initializer_list<std::string_view>);
bool TryReadFiniteFloat(const nlohmann::json&, std::string_view, float&, std::string&);
bool TryParseUnsigned(std::string_view, uint64_t&);
std::optional<ELogLevel> ParseLogLevel(std::string_view);
bool TryParseActorId(std::string_view, FActorId&);
bool TryParseComponentId(std::string_view, FComponentId&);
bool TryParseSpawnRequest(const nlohmann::json&, FAutomationSpawnActorRequest&, std::string&);
bool TryParseTransformPatch(const nlohmann::json&, FAutomationTransformPatch&, std::string&);
}
