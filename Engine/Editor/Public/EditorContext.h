#pragma once

class EditorMode;
struct FEditorViewportState;

struct EditorContext {
  EditorMode* Mode = nullptr;
  FEditorViewportState* Viewport = nullptr;
};
