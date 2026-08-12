#include "PerformanceOverlay.h"

#include <imgui.h>

#include <algorithm>

PerformanceOverlayManager& PerformanceOverlayManager::GetInstance() {
  static PerformanceOverlayManager Instance;
  return Instance;
}

void PerformanceOverlayManager::BeginUpdate() {
  UpdateStartTime = std::chrono::steady_clock::now();
  IsUpdateMeasurementActive = true;
  HasUpdateMeasurement = false;
}

void PerformanceOverlayManager::EndUpdate() {
  if (!IsUpdateMeasurementActive) return;

  const auto EndTime = std::chrono::steady_clock::now();
  CurrentUpdateMs = std::chrono::duration<double, std::milli>(EndTime - UpdateStartTime).count();
  IsUpdateMeasurementActive = false;
  HasUpdateMeasurement = true;
}

void PerformanceOverlayManager::BeginRender() {
  RenderStartTime = std::chrono::steady_clock::now();
  IsRenderMeasurementActive = true;
  HasRenderMeasurement = false;
}

void PerformanceOverlayManager::EndRender() {
  if (!IsRenderMeasurementActive) return;

  const auto EndTime = std::chrono::steady_clock::now();
  CurrentRenderMs = std::chrono::duration<double, std::milli>(EndTime - RenderStartTime).count();
  IsRenderMeasurementActive = false;
  HasRenderMeasurement = true;
}

void PerformanceOverlayManager::CommitFrame(int TargetFps) {
  if (!HasUpdateMeasurement || !HasRenderMeasurement) return;

  CurrentTargetFps = TargetFps;
  HasUpdateMeasurement = false;
  HasRenderMeasurement = false;
  HasCommittedFrame = true;
}

void PerformanceOverlayManager::Update(float DeltaTime) {
  if (!HasCommittedFrame) return;

  AccumulatedUpdateMs += CurrentUpdateMs;
  AccumulatedRenderMs += CurrentRenderMs;
  AccumulatedDisplayTime += (std::max)(0.0f, DeltaTime);
  ++AccumulatedFrameCount;
  HasCommittedFrame = false;

  if (AccumulatedDisplayTime < DisplayUpdateInterval || AccumulatedFrameCount == 0) return;

  const double FrameCount = static_cast<double>(AccumulatedFrameCount);
  Stats.UpdateMs = AccumulatedUpdateMs / FrameCount;
  Stats.RenderMs = AccumulatedRenderMs / FrameCount;
  Stats.FrameMs = Stats.UpdateMs + Stats.RenderMs;
  Stats.LoadPercent = CurrentTargetFps > 0
                          ? Stats.FrameMs / (1000.0 / static_cast<double>(CurrentTargetFps)) * 100.0
                          : 0.0;
  Stats.EstimatedFps = Stats.FrameMs > 0.0 ? 1000.0 / Stats.FrameMs : 0.0;

  AccumulatedUpdateMs = 0.0;
  AccumulatedRenderMs = 0.0;
  AccumulatedDisplayTime = 0.0;
  AccumulatedFrameCount = 0;
}

void PerformanceOverlayManager::Draw() {
  ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::SetNextWindowBgAlpha(0.5f);
  ImGui::Begin("Performance", nullptr, Flags);
  ImGui::Text("Update:      %.2f ms", Stats.UpdateMs);
  ImGui::Text("Render:      %.2f ms", Stats.RenderMs);
  ImGui::Separator();
  ImGui::Text("Frame:       %.2f ms", Stats.FrameMs);
  ImGui::Text("Load:        %.0f %%", Stats.LoadPercent);
  ImGui::Text("Estimated:   %.0f FPS", Stats.EstimatedFps);
  ImGui::End();
}

const FPerformanceStats& PerformanceOverlayManager::GetStats() const { return Stats; }
