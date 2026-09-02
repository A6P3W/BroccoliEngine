#include "Panels/WorldSettingsPanel.h"

#include <imgui.h>

#include <string>
#include <vector>

#include "EditorContext.h"
#include "EditorMode.h"

void WorldSettingsPanel::DrawContents(EditorContext& Context) {
  EditorMode* Mode = Context.Mode;
  const std::vector<std::string>& GameModeClasses = Mode->GetGameModeClassList();
  const std::string& SelectedGameMode = Mode->GetSelectedGameModeClass();
  const char* Preview = SelectedGameMode.empty() ? "(None)" : SelectedGameMode.c_str();

  if (ImGui::BeginCombo("GameMode", Preview)) {
    for (const std::string& ClassName : GameModeClasses) {
      const bool IsSelected = ClassName == SelectedGameMode;
      if (ImGui::Selectable(ClassName.c_str(), IsSelected)) {
        Mode->SetSelectedGameModeClass(ClassName);
      }
      if (IsSelected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}
