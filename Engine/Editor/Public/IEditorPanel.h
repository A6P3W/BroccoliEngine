#pragma once

#include <string_view>

struct EditorContext;

class IEditorPanel {
 public:
  virtual ~IEditorPanel() = default;

  virtual std::string_view GetId() const = 0;
  virtual std::string_view GetTitle() const = 0;

  bool IsVisible() const { return Visible; }
  void SetVisible(bool NewVisible) { Visible = NewVisible; }
  void Draw(EditorContext& Context);

 protected:
  virtual void DrawContents(EditorContext& Context) = 0;

 private:
  bool Visible = true;
};
