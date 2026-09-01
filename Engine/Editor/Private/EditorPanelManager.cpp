#include "EditorPanelManager.h"

#include <string>
#include <utility>

#include "EditorContext.h"
#include "Log.h"

bool EditorPanelManager::RegisterPanel(std::unique_ptr<IEditorPanel> Panel) {
  if (!Panel) {
    M_LOG(Error, "Cannot register a null editor panel.");
    return false;
  }

  const std::string PanelId(Panel->GetId());
  if (PanelId.empty()) {
    M_LOG(Error, "Cannot register an editor panel with an empty ID.");
    return false;
  }
  if (FindPanel(PanelId) != nullptr) {
    M_LOG(Error, "Duplicate editor panel ID: {}", PanelId);
    return false;
  }

  Panels.push_back(std::move(Panel));
  return true;
}

IEditorPanel* EditorPanelManager::FindPanel(std::string_view Id) {
  for (const std::unique_ptr<IEditorPanel>& Panel : Panels) {
    if (Panel->GetId() == Id) return Panel.get();
  }
  return nullptr;
}

const IEditorPanel* EditorPanelManager::FindPanel(std::string_view Id) const {
  for (const std::unique_ptr<IEditorPanel>& Panel : Panels) {
    if (Panel->GetId() == Id) return Panel.get();
  }
  return nullptr;
}

void EditorPanelManager::DrawPanels(EditorContext& Context) {
  for (const std::unique_ptr<IEditorPanel>& Panel : Panels) {
    Panel->Draw(Context);
  }
}
