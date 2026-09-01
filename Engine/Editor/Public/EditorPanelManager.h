#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "IEditorPanel.h"

struct EditorContext;

class EditorPanelManager {
 public:
  bool RegisterPanel(std::unique_ptr<IEditorPanel> Panel);
  IEditorPanel* FindPanel(std::string_view Id);
  const IEditorPanel* FindPanel(std::string_view Id) const;
  void DrawPanels(EditorContext& Context);

  const std::vector<std::unique_ptr<IEditorPanel>>& GetPanels() const { return Panels; }

 private:
  std::vector<std::unique_ptr<IEditorPanel>> Panels;
};
