#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

struct FAutomationActorClassInfo {
  std::string ClassName;
  bool bIsGameMode = false;
};

struct FAutomationLevelInfo {
  uint32_t SceneId = 0;
  std::string LevelPath;
};

using FAutomationActorClassListProvider = std::function<std::vector<FAutomationActorClassInfo>()>;
using FAutomationLevelListProvider = std::function<std::vector<FAutomationLevelInfo>()>;
using FAutomationActorClassExistsProvider = std::function<bool(std::string_view)>;
