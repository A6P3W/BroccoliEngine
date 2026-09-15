#pragma once
#include <nlohmann/json.hpp>
#include "World/WorldTypes.h"
namespace AutomationHttpDetail {
nlohmann::json SerializeActor(const FAutomationActorSnapshot&);
nlohmann::json SerializeActorList(const FAutomationActorListSnapshot&);
}
