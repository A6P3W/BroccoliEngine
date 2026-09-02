#include "Panels/ActionPanel.h"

#include <imgui.h>

#include "EditorContext.h"
#include "EditorMode.h"

void ActionPanel::DrawContents(EditorContext& Context) {
  EditorMode* Mode = Context.Mode;
  int CurrentSelection = static_cast<int>(Mode->GetActorAction());

  if (ImGui::RadioButton("Select", &CurrentSelection, 0)) {
    Mode->SetActorAction(EActorAction::Select);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Move", &CurrentSelection, 1)) {
    Mode->SetActorAction(EActorAction::Move);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", &CurrentSelection, 2)) {
    Mode->SetActorAction(EActorAction::Rotate);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", &CurrentSelection, 3)) {
    Mode->SetActorAction(EActorAction::Scale);
  }
}
