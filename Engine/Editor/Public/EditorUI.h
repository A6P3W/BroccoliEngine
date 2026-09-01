#pragma once

#include "EditorPanelManager.h"

class EditorMode;

class EditorUI {
 public:
  EditorUI();
  void UpdateAndDraw(EditorMode* Mode);

 private:
  void DrawDockSpace();
  void BuildDefaultDockLayout();
  void DrawMenuBar(EditorMode* Mode);
  void DrawCreateNewActorModal(EditorMode* Mode);

  EditorPanelManager PanelManager;
  bool ResetDockLayoutRequested = false;
};
