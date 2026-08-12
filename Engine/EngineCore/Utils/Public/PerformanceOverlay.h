#pragma once

#include <chrono>

#include "BroccoliEngineAPI.h"

struct FPerformanceStats {
  double UpdateMs = 0.0;
  double RenderMs = 0.0;
  double FrameMs = 0.0;
  double LoadPercent = 0.0;
  double EstimatedFps = 0.0;
};

class BROCCOLI_ENGINE_API PerformanceOverlayManager {
 public:
  static PerformanceOverlayManager& GetInstance();

  void BeginUpdate();
  void EndUpdate();

  void BeginRender();
  void EndRender();

  void CommitFrame(int TargetFps);
  void Update(float DeltaTime);
  void Draw();

  const FPerformanceStats& GetStats() const;

 private:
  static constexpr double DisplayUpdateInterval = 0.25;

  FPerformanceStats Stats;
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
