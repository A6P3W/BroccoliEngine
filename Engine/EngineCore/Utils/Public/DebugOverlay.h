#pragma once

#include "BroccoliEngineAPI.h"
#include "UMath.h"

#include <format>
#include <string>
#include <vector>

struct DebugOverlayLogEntry {
  std::string Key;
  float TimeRemaining = 0.0f;
  float DisplayTime = 0.0f;
  std::string Text;
};

struct FWorldLogEntry {
  std::string Key;
  float TimeRemaining = 0.0f;
  float DisplayTime = 0.0f;
  std::string Text;
  FVector2D WorldPosition;
};

class BROCCOLI_ENGINE_API DebugOverlayManager {
 public:
  static DebugOverlayManager& GetInstance();

  void AddLog(const std::string& Key, float DisplayTime, const std::string& FormattedText);
  void AddWorldLog(
      const std::string& Key,
      float DisplayTime,
      const FVector2D& WorldPosition,
      const std::string& FormattedText
  );
  void Update(float DeltaTime);
  void UpdateWorldLogs(float DeltaTime);
  void Draw();
  void DrawWorldLogs();

 private:
  std::vector<DebugOverlayLogEntry> Entries;
  std::vector<FWorldLogEntry> WorldLogs;
};

#define DRAW_SCREEN_LOG(Key, Time, Fmt, ...) \
  DebugOverlayManager::GetInstance().AddLog(Key, Time, std::format(Fmt, ##__VA_ARGS__))

#define DRAW_WORLD_LOG(Key, Time, WorldPosition, Fmt, ...) \
  DebugOverlayManager::GetInstance().AddWorldLog(Key, Time, WorldPosition, std::format(Fmt, ##__VA_ARGS__))
