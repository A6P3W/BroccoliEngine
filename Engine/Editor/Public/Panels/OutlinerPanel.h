#pragma once

#include "IEditorPanel.h"

class OutlinerPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Outliner"; }
  std::string_view GetTitle() const override { return "Outliner"; }

 protected:
  void DrawContents(EditorContext& Context) override;
};
