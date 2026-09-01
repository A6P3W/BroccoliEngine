#include "IEditorPanel.h"

#include <imgui.h>

#include <string>

#include "EditorContext.h"

void IEditorPanel::Draw(EditorContext& Context) {
  if (!Visible) return;

  const std::string WindowName = std::string(GetTitle()) + "###" + std::string(GetId());
  if (ImGui::Begin(WindowName.c_str(), &Visible)) {
    DrawContents(Context);
  }
  ImGui::End();
}
