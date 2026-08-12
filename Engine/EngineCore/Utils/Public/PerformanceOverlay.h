#pragma once

#include <array>
#include <chrono>

#include "BroccoliEngineAPI.h"

struct FUpdatePerformanceStats {
  double SceneMs = 0.0;
  double AutomationMs = 0.0;
  double EOSMs = 0.0;
  double NetworkMs = 0.0;
  double InputMs = 0.0;
  double HttpMs = 0.0;
  double DebugOverlayMs = 0.0;
  double AudioMs = 0.0;
  double WorldMs = 0.0;
};

struct FWorldPerformanceStats {
  double ActorsMs = 0.0;
  double ReplicationMs = 0.0;
  double ActorCleanupMs = 0.0;
  double ActorSpawnFlushMs = 0.0;
  double TimersMs = 0.0;
  double CollisionMs = 0.0;
};

struct FPerformanceStats {
  double UpdateMs = 0.0;
  double RenderMs = 0.0;
  double FrameMs = 0.0;
  double LoadPercent = 0.0;
  double EstimatedFps = 0.0;
  FUpdatePerformanceStats UpdateDetail;
  FWorldPerformanceStats WorldDetail;
};

enum class EPerformanceSection {
  Scene,
  Automation,
  EOS,
  Network,
  Input,
  Http,
  DebugOverlay,
  Audio,
  World,
  WorldActors,
  WorldReplication,
  WorldActorCleanup,
  WorldActorSpawnFlush,
  WorldTimers,
  WorldCollision,
  Count,
};

class BROCCOLI_ENGINE_API PerformanceOverlayManager {
 public:
  static PerformanceOverlayManager& GetInstance();

  void BeginUpdate();
  void EndUpdate();

  void BeginRender();
  void EndRender();

  void BeginSection(EPerformanceSection Section);
  void EndSection(EPerformanceSection Section);

  void CommitFrame(int TargetFps);
  void Update(float DeltaTime);
  void Draw();

  const FPerformanceStats& GetStats() const;

 private:
  struct FPerformanceSectionMeasurement {
    double CurrentMs = 0.0;
    double AccumulatedMs = 0.0;
    std::chrono::steady_clock::time_point StartTime;
    bool IsActive = false;
  };

  static constexpr double DisplayUpdateInterval = 0.25;
  static constexpr std::size_t SectionCount = static_cast<std::size_t>(EPerformanceSection::Count);

  static std::size_t GetSectionIndex(EPerformanceSection Section);
  void ResetCurrentSectionMeasurements();
  void UpdateDisplayedSectionStats(double FrameCount);
  void DrawSummary();
  void DrawUpdateDetails();
  void DrawWorldDetails();
  void DrawSection(const char* Label, double Milliseconds) const;

  FPerformanceStats Stats;
  std::array<FPerformanceSectionMeasurement, SectionCount> SectionMeasurements;
  std::chrono::steady_clock::time_point UpdateStartTime;
  std::chrono::steady_clock::time_point RenderStartTime;
  double CurrentUpdateMs = 0.0;
  double CurrentRenderMs = 0.0;
  double AccumulatedUpdateMs = 0.0;
  double AccumulatedRenderMs = 0.0;
  double AccumulatedDisplayTime = 0.0;
  int CurrentTargetFps = 0;
  unsigned int AccumulatedFrameCount = 0;
  bool IsUpdateMeasurementActive = false;
  bool IsRenderMeasurementActive = false;
  bool HasUpdateMeasurement = false;
  bool HasRenderMeasurement = false;
  bool HasCommittedFrame = false;
};
