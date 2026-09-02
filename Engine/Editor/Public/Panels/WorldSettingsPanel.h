#pragma once

#include "IEditorPanel.h"

class WorldSettingsPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "World Settings"; }
  std::string_view GetTitle() const override { return "World Settings"; }

 protected:
  void DrawContents(EditorContext& Context) override;
};
