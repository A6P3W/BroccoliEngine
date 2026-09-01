#pragma once

#include "IEditorPanel.h"

class ActionPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Action"; }
  std::string_view GetTitle() const override { return "Action"; }

 protected:
  void DrawContents(EditorContext& Context) override;
};
