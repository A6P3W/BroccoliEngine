#include "EditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "ActorClassGenerator.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "FileUtils.h"
#include "Log.h"
#include "Panels/ActionPanel.h"
#include "Panels/ClassBrowserPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/OutlinerPanel.h"
#include "Panels/WorldSettingsPanel.h"
#include "PathResolver.h"

EditorUI::EditorUI() {
  PanelManager.RegisterPanel(std::make_unique<ClassBrowserPanel>());
  PanelManager.RegisterPanel(std::make_unique<OutlinerPanel>());
  PanelManager.RegisterPanel(std::make_unique<InspectorPanel>());
  PanelManager.RegisterPanel(std::make_unique<WorldSettingsPanel>());
  PanelManager.RegisterPanel(std::make_unique<ActionPanel>());
}

void EditorUI::UpdateAndDraw(EditorMode* Mode) {
  DrawMenuBar(Mode);
  DrawDockSpace();

  EditorContext Context{.Mode = Mode};
  PanelManager.DrawPanels(Context);
  DrawCreateNewActorModal(Mode);
}

void EditorUI::DrawDockSpace() {
  ImGuiViewport* MainViewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(MainViewport->WorkPos);
  ImGui::SetNextWindowSize(MainViewport->WorkSize);
  ImGui::SetNextWindowViewport(MainViewport->ID);

  constexpr ImGuiWindowFlags WindowFlags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
  constexpr ImGuiDockNodeFlags DockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("BroccoliEditorDockSpaceHost", nullptr, WindowFlags);
  ImGui::PopStyleVar(3);

  const ImGuiID DockSpaceId = ImGui::GetID("BroccoliEditorDockSpace");
  const bool HasSavedLayout = ImGui::DockBuilderGetNode(DockSpaceId) != nullptr;
  ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), DockSpaceFlags);

  if (!HasSavedLayout || ResetDockLayoutRequested) {
    BuildDefaultDockLayout();
    ResetDockLayoutRequested = false;
  }

  ImGui::End();
}

void EditorUI::BuildDefaultDockLayout() {
  const ImGuiID DockSpaceId = ImGui::GetID("BroccoliEditorDockSpace");
  const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

  ImGui::DockBuilderRemoveNode(DockSpaceId);
  ImGui::DockBuilderAddNode(
      DockSpaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode
  );
  ImGui::DockBuilderSetNodeSize(DockSpaceId, MainViewport->WorkSize);

  ImGuiID CenterNodeId = DockSpaceId;
  ImGuiID LeftNodeId =
      ImGui::DockBuilderSplitNode(CenterNodeId, ImGuiDir_Left, 0.21f, nullptr, &CenterNodeId);
  ImGuiID RightNodeId =
      ImGui::DockBuilderSplitNode(CenterNodeId, ImGuiDir_Right, 0.25f, nullptr, &CenterNodeId);
  const ImGuiID BottomNodeId =
      ImGui::DockBuilderSplitNode(CenterNodeId, ImGuiDir_Down, 0.13f, nullptr, &CenterNodeId);

  ImGuiID ClassBrowserNodeId = LeftNodeId;
  const ImGuiID OutlinerNodeId = ImGui::DockBuilderSplitNode(
      ClassBrowserNodeId, ImGuiDir_Down, 0.45f, nullptr, &ClassBrowserNodeId
  );
  ImGuiID InspectorNodeId = RightNodeId;
  const ImGuiID WorldSettingsNodeId =
      ImGui::DockBuilderSplitNode(InspectorNodeId, ImGuiDir_Down, 0.35f, nullptr, &InspectorNodeId);

  ImGui::DockBuilderDockWindow("Class Browser", ClassBrowserNodeId);
  ImGui::DockBuilderDockWindow("Outliner", OutlinerNodeId);
  ImGui::DockBuilderDockWindow("Inspector", InspectorNodeId);
  ImGui::DockBuilderDockWindow("World Settings", WorldSettingsNodeId);
  ImGui::DockBuilderDockWindow("Action", BottomNodeId);
  ImGui::DockBuilderFinish(DockSpaceId);
}

void EditorUI::DrawMenuBar(EditorMode* Mode) {
  bool OpenCreateNewActorPopup = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Level")) {
        const std::string FilePath = FileDialog::SaveFile(
            "Broccoli Level JSON (*.BLevel.json)\0*.BLevel.json\0All Files (*.*)\0*.*\0",
            "BLevel.json",
            PathResolver::GetGameResourceDir()
        );
        if (!FilePath.empty()) Mode->SaveLevel(FilePath);
      }
      if (ImGui::MenuItem("Load Level")) {
        const std::string FilePath = FileDialog::OpenFile(
            "Broccoli Level Files (*.BLevel;*.BLevel.json)\0*.BLevel;*.BLevel.json\0All Files "
            "(*.*)\0*.*\0",
            PathResolver::GetGameResourceDir()
        );
        if (!FilePath.empty()) Mode->LoadLevel(FilePath);
      }
      if (ImGui::MenuItem("Create New Actor")) OpenCreateNewActorPopup = true;
      if (ImGui::MenuItem("Simulate")) Mode->Simulate();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      for (const std::unique_ptr<IEditorPanel>& Panel : PanelManager.GetPanels()) {
        const std::string Title(Panel->GetTitle());
        const bool IsVisible = Panel->IsVisible();
        if (ImGui::MenuItem(Title.c_str(), nullptr, IsVisible)) Panel->SetVisible(!IsVisible);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Reset Layout")) {
        for (const std::unique_ptr<IEditorPanel>& Panel : PanelManager.GetPanels()) {
          Panel->SetVisible(true);
        }
        ResetDockLayoutRequested = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  if (OpenCreateNewActorPopup) ImGui::OpenPopup("Create New Actor");
}

void EditorUI::DrawCreateNewActorModal(EditorMode* Mode) {
  static char ClassName[128] = {};
  static std::string ParentClassName;
  static FCreateNewActorResult LastResult;
  static ActorClassGenerator Generator;

  ImGui::SetNextWindowSize(ImVec2(620.0f, 0.0f), ImGuiCond_Appearing);
  if (!ImGui::BeginPopupModal("Create New Actor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  std::vector<std::string> ParentClasses = Mode->GetClassList();
  std::sort(ParentClasses.begin(), ParentClasses.end());
  if (ImGui::IsWindowAppearing()) {
    if (std::find(ParentClasses.begin(), ParentClasses.end(), ParentClassName) ==
        ParentClasses.end()) {
      ParentClassName = ParentClasses.empty() ? "" : ParentClasses.front();
    }
    LastResult = {};
  }

  ImGui::InputText("Class Name", ClassName, sizeof(ClassName));
  const char* ParentPreview = ParentClassName.empty() ? "(None)" : ParentClassName.c_str();
  if (ImGui::BeginCombo("Parent Actor Class", ParentPreview)) {
    for (const std::string& Candidate : ParentClasses) {
      const bool IsSelected = Candidate == ParentClassName;
      if (ImGui::Selectable(Candidate.c_str(), IsSelected)) ParentClassName = Candidate;
      if (IsSelected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (!LastResult.Message.empty()) {
    ImGui::Separator();
    const ImVec4 MessageColor =
        LastResult.bSuccess ? ImVec4(0.35f, 0.85f, 0.35f, 1.0f) : ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, MessageColor);
    ImGui::TextWrapped("%s", LastResult.Message.c_str());
    ImGui::PopStyleColor();
  }

  ImGui::Separator();
  if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
    const std::string SelectedPath = FileDialog::SelectFolder();
    if (!SelectedPath.empty()) {
      FCreateNewActorRequest Request;
      Request.ClassName = ClassName;
      Request.ParentClassName = ParentClassName;
      Request.OutputDirectory = FileUtils::Utf8ToPath(SelectedPath);
      LastResult = Generator.Generate(Request);
      M_LOG(Log, "{}", LastResult.Message);
      if (LastResult.bSuccess) std::memset(ClassName, 0, sizeof(ClassName));
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) ImGui::CloseCurrentPopup();

  ImGui::EndPopup();
}
