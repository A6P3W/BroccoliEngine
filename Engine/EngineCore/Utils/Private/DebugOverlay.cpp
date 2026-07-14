#include "DebugOverlay.h"

#include <algorithm>

#include <imgui.h>

#include "EngineDefine.h"
#include "RenderSystem.h"

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
    EntryIt->DisplayTime = DisplayTime;
    EntryIt->Text = FormattedText;
    return;
  }

  Entries.push_back({Key, DisplayTime, DisplayTime, FormattedText});
}


void DebugOverlayManager::AddWorldLog(
    const std::string& Key,
    float DisplayTime,
    const FVector2D& WorldPosition,
    const std::string& FormattedText
) {
  const auto EntryIt = std::find_if(
      WorldLogs.begin(),
      WorldLogs.end(),
      [&Key](const FWorldLogEntry& Entry) { return Entry.Key == Key; }
  );

  if (EntryIt != WorldLogs.end()) {
    EntryIt->TimeRemaining = DisplayTime;
    EntryIt->DisplayTime = DisplayTime;
    EntryIt->WorldPosition = WorldPosition;
    EntryIt->Text = FormattedText;
    return;
  }

  WorldLogs.push_back({Key, DisplayTime, DisplayTime, FormattedText, WorldPosition});
}
void DebugOverlayManager::Update(float DeltaTime) {
  for (DebugOverlayLogEntry& Entry : Entries) {
    Entry.TimeRemaining -= DeltaTime;
  }

  std::erase_if(Entries, [](const DebugOverlayLogEntry& Entry) {
    return Entry.TimeRemaining <= 0.0f;
  });

  UpdateWorldLogs(DeltaTime);
}

void DebugOverlayManager::UpdateWorldLogs(float DeltaTime) {
  for (FWorldLogEntry& Entry : WorldLogs) {
    Entry.TimeRemaining -= DeltaTime;
  }

  std::erase_if(WorldLogs, [](const FWorldLogEntry& Entry) {
    return Entry.TimeRemaining <= 0.0f;
  });
}

void DebugOverlayManager::Draw() {
  ImGui::Begin("Debug Overlay");

  constexpr float FadeStartTime = 0.5f;
  for (const DebugOverlayLogEntry& Entry : Entries) {
    const float FadeTime = std::max(0.001f, std::min(FadeStartTime, Entry.DisplayTime));
    const float Alpha = std::clamp(Entry.TimeRemaining / FadeTime, 0.0f, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
    ImGui::Text("%s", Entry.Text.c_str());
    ImGui::PopStyleVar();
  }

  ImGui::End();
  DrawWorldLogs();
}
void DebugOverlayManager::DrawWorldLogs() {
  ImDrawList* DrawList = ImGui::GetForegroundDrawList();
  const ImVec2 DisplaySize = ImGui::GetIO().DisplaySize;
  const float Scale = std::min(DisplaySize.x / VirtualWidth, DisplaySize.y / VirtualHeight);
  const ImVec2 Offset = {
      (DisplaySize.x - VirtualWidth * Scale) * 0.5f,
      (DisplaySize.y - VirtualHeight * Scale) * 0.5f
  };

  constexpr float FadeStartTime = 0.5f;
  for (const FWorldLogEntry& Entry : WorldLogs) {
    const FVector2D WorldScreenPosition =
        RenderSystem::GetInstance().WorldToScreen(Entry.WorldPosition);
    const ImVec2 TextSize = ImGui::CalcTextSize(Entry.Text.c_str());
    const ImVec2 TextPosition = {
        Offset.x + WorldScreenPosition.X * Scale - TextSize.x * 0.5f,
        Offset.y + WorldScreenPosition.Y * Scale - TextSize.y * 0.5f
    };
    const float FadeTime = std::max(0.001f, std::min(FadeStartTime, Entry.DisplayTime));
    const float Alpha = std::clamp(Entry.TimeRemaining / FadeTime, 0.0f, 1.0f);
    const ImU32 TextColor = IM_COL32(255, 255, 255, static_cast<int>(Alpha * 255.0f));
    const ImU32 OutlineColor = IM_COL32(0, 0, 0, static_cast<int>(Alpha * 255.0f));

    DrawList->AddText({TextPosition.x - 1.0f, TextPosition.y}, OutlineColor, Entry.Text.c_str());
    DrawList->AddText({TextPosition.x + 1.0f, TextPosition.y}, OutlineColor, Entry.Text.c_str());
    DrawList->AddText({TextPosition.x, TextPosition.y - 1.0f}, OutlineColor, Entry.Text.c_str());
    DrawList->AddText({TextPosition.x, TextPosition.y + 1.0f}, OutlineColor, Entry.Text.c_str());
    DrawList->AddText(TextPosition, TextColor, Entry.Text.c_str());
  }
}