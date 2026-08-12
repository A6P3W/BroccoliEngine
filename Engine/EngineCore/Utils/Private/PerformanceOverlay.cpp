#include "PerformanceOverlay.h"

#include <imgui.h>

#include <algorithm>

namespace {
double GetPercentage(double Milliseconds, double TotalMilliseconds) {
  return TotalMilliseconds > 0.0 ? Milliseconds / TotalMilliseconds * 100.0 : 0.0;
}
}  // namespace

PerformanceOverlayManager& PerformanceOverlayManager::GetInstance() {
  static PerformanceOverlayManager Instance;
  return Instance;
}

void PerformanceOverlayManager::BeginUpdate() {
  ResetCurrentSectionMeasurements();
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

void PerformanceOverlayManager::BeginSection(EPerformanceSection Section) {
  FPerformanceSectionMeasurement& Measurement = SectionMeasurements[GetSectionIndex(Section)];
  Measurement.StartTime = std::chrono::steady_clock::now();
  Measurement.IsActive = true;
}

void PerformanceOverlayManager::EndSection(EPerformanceSection Section) {
  FPerformanceSectionMeasurement& Measurement = SectionMeasurements[GetSectionIndex(Section)];
  if (!Measurement.IsActive) return;

  const auto EndTime = std::chrono::steady_clock::now();
  Measurement.CurrentMs =
      std::chrono::duration<double, std::milli>(EndTime - Measurement.StartTime).count();
  Measurement.IsActive = false;
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
  for (FPerformanceSectionMeasurement& Measurement : SectionMeasurements) {
    Measurement.AccumulatedMs += Measurement.CurrentMs;
  }
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
  UpdateDisplayedSectionStats(FrameCount);

  AccumulatedUpdateMs = 0.0;
  AccumulatedRenderMs = 0.0;
  for (FPerformanceSectionMeasurement& Measurement : SectionMeasurements) {
    Measurement.AccumulatedMs = 0.0;
  }
  AccumulatedDisplayTime = 0.0;
  AccumulatedFrameCount = 0;
}

void PerformanceOverlayManager::Draw() {
  ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::SetNextWindowBgAlpha(0.5f);
  ImGui::Begin("Performance", nullptr, Flags);
  DrawSummary();
  ImGui::Separator();
  ImGui::Text("Frame:       %.2f ms", Stats.FrameMs);
  ImGui::Text("Load:        %.0f %%", Stats.LoadPercent);
  ImGui::Text("Estimated:   %.0f FPS", Stats.EstimatedFps);
  ImGui::End();
}

const FPerformanceStats& PerformanceOverlayManager::GetStats() const { return Stats; }

std::size_t PerformanceOverlayManager::GetSectionIndex(EPerformanceSection Section) {
  return static_cast<std::size_t>(Section);
}

void PerformanceOverlayManager::ResetCurrentSectionMeasurements() {
  for (FPerformanceSectionMeasurement& Measurement : SectionMeasurements) {
    Measurement.CurrentMs = 0.0;
    Measurement.IsActive = false;
  }
}

void PerformanceOverlayManager::UpdateDisplayedSectionStats(double FrameCount) {
  const auto GetAverage = [this, FrameCount](EPerformanceSection Section) {
    return SectionMeasurements[GetSectionIndex(Section)].AccumulatedMs / FrameCount;
  };

  Stats.UpdateDetail.SceneMs = GetAverage(EPerformanceSection::Scene);
  Stats.UpdateDetail.AutomationMs = GetAverage(EPerformanceSection::Automation);
  Stats.UpdateDetail.EOSMs = GetAverage(EPerformanceSection::EOS);
  Stats.UpdateDetail.NetworkMs = GetAverage(EPerformanceSection::Network);
  Stats.UpdateDetail.InputMs = GetAverage(EPerformanceSection::Input);
  Stats.UpdateDetail.HttpMs = GetAverage(EPerformanceSection::Http);
  Stats.UpdateDetail.DebugOverlayMs = GetAverage(EPerformanceSection::DebugOverlay);
  Stats.UpdateDetail.AudioMs = GetAverage(EPerformanceSection::Audio);
  Stats.UpdateDetail.WorldMs = GetAverage(EPerformanceSection::World);

  Stats.WorldDetail.ActorsMs = GetAverage(EPerformanceSection::WorldActors);
  Stats.WorldDetail.ReplicationMs = GetAverage(EPerformanceSection::WorldReplication);
  Stats.WorldDetail.ActorCleanupMs = GetAverage(EPerformanceSection::WorldActorCleanup);
  Stats.WorldDetail.ActorSpawnFlushMs = GetAverage(EPerformanceSection::WorldActorSpawnFlush);
  Stats.WorldDetail.TimersMs = GetAverage(EPerformanceSection::WorldTimers);
  Stats.WorldDetail.CollisionMs = GetAverage(EPerformanceSection::WorldCollision);
}

void PerformanceOverlayManager::DrawSummary() {
  if (!ImGui::BeginTable("PerformanceSummary", 3)) return;

  ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("Milliseconds", ImGuiTableColumnFlags_WidthFixed, 70.0f);
  ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, 50.0f);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  const bool IsUpdateOpen = ImGui::TreeNodeEx(
      "Update", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Update"
  );
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%.2f ms", Stats.UpdateMs);
  ImGui::EndTable();
  if (IsUpdateOpen) {
    ImGui::TreePush("UpdateDetails");
    DrawUpdateDetails();
    ImGui::TreePop();
  }

  if (!ImGui::BeginTable("PerformanceRender", 3)) return;

  ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("Milliseconds", ImGuiTableColumnFlags_WidthFixed, 70.0f);
  ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, 50.0f);
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::TextUnformatted("Render");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%.2f ms", Stats.RenderMs);
  ImGui::EndTable();
}

void PerformanceOverlayManager::DrawUpdateDetails() {
  if (!ImGui::BeginTable("UpdatePerformanceDetails", 3)) return;

  ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("Milliseconds", ImGuiTableColumnFlags_WidthFixed, 70.0f);
  ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, 50.0f);

  DrawSection("Scene", Stats.UpdateDetail.SceneMs);
  DrawSection("Automation", Stats.UpdateDetail.AutomationMs);
  DrawSection("EOS", Stats.UpdateDetail.EOSMs);
  DrawSection("Network", Stats.UpdateDetail.NetworkMs);
  DrawSection("Input", Stats.UpdateDetail.InputMs);
  DrawSection("HTTP", Stats.UpdateDetail.HttpMs);
  DrawSection("Debug", Stats.UpdateDetail.DebugOverlayMs);
  DrawSection("Audio", Stats.UpdateDetail.AudioMs);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  const bool IsWorldOpen = ImGui::TreeNodeEx(
      "World", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen, "World"
  );
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%.2f ms", Stats.UpdateDetail.WorldMs);
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%.0f%%", GetPercentage(Stats.UpdateDetail.WorldMs, Stats.UpdateMs));

  ImGui::EndTable();
  if (IsWorldOpen) {
    ImGui::TreePush("WorldDetails");
    DrawWorldDetails();
    ImGui::TreePop();
  }
}

void PerformanceOverlayManager::DrawWorldDetails() {
  if (!ImGui::BeginTable("WorldPerformanceDetails", 3)) return;

  ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 100.0f);
  ImGui::TableSetupColumn("Milliseconds", ImGuiTableColumnFlags_WidthFixed, 70.0f);
  ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, 50.0f);

  DrawSection("Actors", Stats.WorldDetail.ActorsMs);
  DrawSection("Replication", Stats.WorldDetail.ReplicationMs);
  DrawSection("Cleanup", Stats.WorldDetail.ActorCleanupMs);
  DrawSection("SpawnFlush", Stats.WorldDetail.ActorSpawnFlushMs);
  DrawSection("Timers", Stats.WorldDetail.TimersMs);
  DrawSection("Collision", Stats.WorldDetail.CollisionMs);
  ImGui::EndTable();
}

void PerformanceOverlayManager::DrawSection(const char* Label, double Milliseconds) const {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::TextUnformatted(Label);
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%.2f ms", Milliseconds);
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%.0f%%", GetPercentage(Milliseconds, Stats.UpdateMs));
}
