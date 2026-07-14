#pragma once

#include "BroccoliEngineAPI.h"

#include <format>
#include <string>
#include <vector>

struct DebugOverlayLogEntry {
  std::string Key;
  float TimeRemaining = 0.0f;
  std::string Text;
};

class BROCCOLI_ENGINE_API DebugOverlayManager {
 public:
  static DebugOverlayManager& GetInstance();

  void AddLog(const std::string& Key, float DisplayTime, const std::string& FormattedText);
  void Update(float DeltaTime);
  void Draw();

 private:
  std::vector<DebugOverlayLogEntry> Entries;
};

#define DrawScreenLog(Key, Time, Fmt, ...) \
  DebugOverlayManager::GetInstance().AddLog(Key, Time, std::format(Fmt, ##__VA_ARGS__))
