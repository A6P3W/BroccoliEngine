#pragma once

#include <string>
#include <vector>

#include "IEditorPanel.h"

class ClassBrowserPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Class Browser"; }
  std::string_view GetTitle() const override { return "Class Browser"; }

 protected:
  void DrawContents(EditorContext& Context) override;

 private:
  std::vector<std::string> RecentClasses;
  int GroupCharacterCount = 2;
};
