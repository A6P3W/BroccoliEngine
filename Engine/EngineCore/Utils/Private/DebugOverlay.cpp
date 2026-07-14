#include "DebugOverlay.h"

#include <algorithm>

#include <imgui.h>

DebugOverlayManager& DebugOverlayManager::GetInstance() {
  static DebugOverlayManager Instance;
  return Instance;
}

void DebugOverlayManager::AddLog(
    const std::string& Key,
    float DisplayTime,
    const std::string& FormattedText
) {
  const auto EntryIt = std::find_if(
      Entries.begin(),
      Entries.end(),
      [&Key](const DebugOverlayLogEntry& Entry) { return Entry.Key == Key; }
  );

  if (EntryIt != Entries.end()) {
    EntryIt->TimeRemaining = DisplayTime;
    EntryIt->Text = FormattedText;
    return;
  }

  Entries.push_back({Key, DisplayTime, FormattedText});
}

void DebugOverlayManager::Update(float DeltaTime) {
  for (DebugOverlayLogEntry& Entry : Entries) {
    Entry.TimeRemaining -= DeltaTime;
  }

  std::erase_if(Entries, [](const DebugOverlayLogEntry& Entry) {
    return Entry.TimeRemaining <= 0.0f;
  });
}

void DebugOverlayManager::Draw() {
  ImGui::Begin("Debug Overlay");

  constexpr float FadeStartTime = 0.5f;
  for (const DebugOverlayLogEntry& Entry : Entries) {
    const float Alpha = std::clamp(Entry.TimeRemaining / FadeStartTime, 0.0f, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
    ImGui::Text("%s", Entry.Text.c_str());
    ImGui::PopStyleVar();
  }

  ImGui::End();
}