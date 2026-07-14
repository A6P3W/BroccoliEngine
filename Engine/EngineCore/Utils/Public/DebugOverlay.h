#pragma once

#include "BroccoliEngineAPI.h"
#include "UMath.h"

#include <format>
#include <string>
#include <unordered_map>
#include <vector>

class AActor;

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
  void AddWorldLog(
      const std::string& Key,
      float DisplayTime,
      const AActor* Actor,
      const std::string& FormattedText
  );
  void Update(float DeltaTime);
  void UpdateWorldLogs(float DeltaTime);
  void Draw();
  void DrawWorldLogs();

 private:
  std::vector<DebugOverlayLogEntry> Entries;
  std::vector<FWorldLogEntry> WorldLogs;
  std::unordered_map<const AActor*, std::size_t> ActorWorldLogCounts;
};

#define DRAW_SCREEN_LOG(Key, Time, Fmt, ...) \
  DebugOverlayManager::GetInstance().AddLog(Key, Time, std::format(Fmt, ##__VA_ARGS__))

#define DRAW_WORLD_LOG(Key, Time, Target, Fmt, ...) \
  DebugOverlayManager::GetInstance().AddWorldLog(Key, Time, Target, std::format(Fmt, ##__VA_ARGS__))