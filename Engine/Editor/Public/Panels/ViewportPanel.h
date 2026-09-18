#pragma once

#include <string>

#include "IEditorPanel.h"

class ViewportPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Viewport"; }
  std::string_view GetTitle() const override { return "Viewport"; }

 protected:
  void DrawContents(EditorContext& Context) override;

 private:
  void CreateStaticMeshActor(EditorContext& Context, const std::string& ModelPath) const;
};
