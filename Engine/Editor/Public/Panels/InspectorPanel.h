#pragma once

#include "IEditorPanel.h"

class InspectorPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Inspector"; }
  std::string_view GetTitle() const override { return "Inspector"; }

 protected:
  void DrawContents(EditorContext& Context) override;
};
