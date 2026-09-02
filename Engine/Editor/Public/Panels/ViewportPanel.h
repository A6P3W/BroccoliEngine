#pragma once

#include "IEditorPanel.h"

class ViewportPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Viewport"; }
  std::string_view GetTitle() const override { return "Viewport"; }

 protected:
  void DrawContents(EditorContext& Context) override;
};
