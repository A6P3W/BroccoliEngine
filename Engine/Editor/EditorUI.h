#pragma once

class EditorMode;

class EditorUI {
public:
	void UpdateAndDraw(EditorMode* editorMode);

private:
	void DrawMenuBar(EditorMode* editorMode);
	void DrawClassBrowser(EditorMode* editorMode);
	void DrawOutliner(EditorMode* editorMode);
	void DrawInspector(EditorMode* editorMode);
};
