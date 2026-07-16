#include "EditorUI.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "Actor.h"
#include "ActorClassGenerator.h"
#include "ActorManager.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "Log.h"
#include "SpriteActor.h"
#include "UMath.h"
#include "World.h"

namespace {
std::filesystem::path Utf8ToPath(const std::string& Value) {
  return std::filesystem::path(reinterpret_cast<const char8_t*>(Value.c_str()));
}

}  // namespace

void EditorUI::UpdateAndDraw(EditorMode* editorMode) {
  DrawMenuBar(editorMode);
  DrawCreateNewActorModal(editorMode);

  DrawClassBrowser(editorMode);
  DrawOutliner(editorMode);
  DrawInspector(editorMode);
  DrawWorldSettings(editorMode);

  DrawSelectAction(editorMode);
}

void EditorUI::DrawMenuBar(EditorMode* editorMode) {
  bool bOpenCreateNewActorPopup = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Level")) {
        // 保存ダイアログを開く
        std::string filepath = FileDialog::SaveFile(
            "Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0", "BLevel"
        );
        if (!filepath.empty()) {
          editorMode->SaveLevel(filepath);
        }
      }
      if (ImGui::MenuItem("Load Level")) {
        // 開くダイアログを開く
        std::string filepath = FileDialog::OpenFile(
            "Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0"
        );
        if (!filepath.empty()) {
          editorMode->LoadLevel(filepath);
        }
      }
      if (ImGui::MenuItem("Create New Actor")) {
        bOpenCreateNewActorPopup = true;
      }
      if (ImGui::MenuItem("Simulate")) {
        editorMode->Simulate();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  if (bOpenCreateNewActorPopup) {
    ImGui::OpenPopup("Create New Actor");
  }
}

void EditorUI::DrawCreateNewActorModal(EditorMode* editorMode) {
  static char ClassName[128] = {};
  static std::string ParentClassName;
  static FCreateNewActorResult LastResult;
  static ActorClassGenerator Generator;

  ImGui::SetNextWindowSize(ImVec2(620.0f, 0.0f), ImGuiCond_Appearing);
  if (!ImGui::BeginPopupModal("Create New Actor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  std::vector<std::string> ParentClasses = editorMode->GetClassList();
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
      const bool bSelected = Candidate == ParentClassName;
      if (ImGui::Selectable(Candidate.c_str(), bSelected)) {
        ParentClassName = Candidate;
      }
      if (bSelected) {
        ImGui::SetItemDefaultFocus();
      }
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
      Request.OutputDirectory = Utf8ToPath(SelectedPath);
      LastResult = Generator.Generate(Request);
      M_LOG("{}", LastResult.Message);
      if (LastResult.bSuccess) {
        std::memset(ClassName, 0, sizeof(ClassName));
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();
}

void EditorUI::DrawClassBrowser(EditorMode* editorMode) {
  ImGui::Begin("Class Browser");

  const std::string& selectedClass = editorMode->GetSelectedClass();

  std::vector<std::string> sortedClasses = editorMode->GetClassList();
  std::sort(sortedClasses.begin(), sortedClasses.end());

  static std::vector<std::string> recentClasses;

  if (recentClasses.empty() && !sortedClasses.empty()) {
    size_t initCount = (sortedClasses.size() < 5) ? sortedClasses.size() : 5;
    for (size_t i = 0; i < initCount; ++i) {
      recentClasses.push_back(sortedClasses[i]);
    }
  }

  auto OnClassSelected = [&](const std::string& className) {
    editorMode->SelectClass(className);

    auto it = std::find(recentClasses.begin(), recentClasses.end(), className);
    if (it == recentClasses.end()) {
      recentClasses.insert(recentClasses.begin(), className);
      if (recentClasses.size() > 5) {
        recentClasses.pop_back();
      }
    }
  };

  ImGui::Text("Selected:");
  ImGui::SameLine();
  if (selectedClass.empty()) {
    ImGui::TextDisabled("(None)");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[ %s ]", selectedClass.c_str());
  }

  if (ImGui::Button("Clear Selection", ImVec2(-1, 0))) {
    editorMode->SelectClass("");
  }

  ImGui::Separator();

  static ImGuiTextFilter filter;
  filter.Draw("##Search", ImGui::GetContentRegionAvail().x);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Search classes...");
  }

  static int groupCharCount = 2;
  ImGui::SliderInt("Group Length", &groupCharCount, 1, 4, "%d chars");

  ImGui::Separator();

  auto GetHueFromName = [](const std::string& name) -> float {
    unsigned long hash = 5381;
    for (char c : name) {
      hash = ((hash << 5) + hash) + c;
    }
    return static_cast<float>(hash % 1000) / 1000.0f;
  };

  auto DrawClassSelectable = [&](const std::string& className) {
    bool isSelected = (className == selectedClass);

    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(GetHueFromName(className), 0.4f, 0.9f, r, g, b);

    if (!isSelected) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(r, g, b, 1.0f));
    }

    if (ImGui::Selectable(className.c_str(), isSelected)) {
      OnClassSelected(className);
    }

    if (!isSelected) {
      ImGui::PopStyleColor();
    }

    if (isSelected) {
      ImGui::SetItemDefaultFocus();
    }
  };

  ImGui::BeginChild("ClassListRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

  if (filter.IsActive()) {
    for (const auto& className : sortedClasses) {
      if (filter.PassFilter(className.c_str())) {
        DrawClassSelectable(className);
      }
    }
  } else {
    // --- Recent グループ ---
    if (!recentClasses.empty()) {
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);
      if (ImGui::TreeNode("Recent")) {
        for (const auto& className : recentClasses) {
          if (std::find(sortedClasses.begin(), sortedClasses.end(), className) !=
              sortedClasses.end()) {
            DrawClassSelectable(className);
          }
        }
        ImGui::TreePop();
      }
      ImGui::Separator();
    }

    // --- ツリーグループ ---
    auto GetGroupKey = [&](const std::string& name) -> std::string {
      if (name.empty()) return "?";

      size_t nameLen = name.length();
      size_t limit = static_cast<size_t>(groupCharCount);
      size_t count = (nameLen < limit) ? nameLen : limit;

      std::string key = name.substr(0, count);
      for (char& c : key) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      }
      return key;
    };

    std::string currentGroup = "";
    bool nodeOpen = false;

    for (const auto& className : sortedClasses) {
      std::string groupKey = GetGroupKey(className);

      if (groupKey != currentGroup) {
        if (!currentGroup.empty() && nodeOpen) {
          ImGui::TreePop();
        }
        currentGroup = groupKey;

        std::string headerName = std::string("[ ") + currentGroup + " ]";
        nodeOpen = ImGui::TreeNodeEx(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
      }

      if (nodeOpen) {
        DrawClassSelectable(className);
      }
    }

    if (!currentGroup.empty() && nodeOpen) {
      ImGui::TreePop();
    }
  }

  ImGui::EndChild();
  ImGui::End();
}

void EditorUI::DrawOutliner(EditorMode* editorMode) {
  ImGui::Begin("Outliner");

  const auto& actors = editorMode->GetWorld()->GetActorManager()->GetAllActors();
  for (size_t i = 0; i < actors.size(); ++i) {
    AActor* actor = actors[i].get();
    if (!actor || actor->IsPendingDestroy()) continue;

    std::string label = actor->GetActorClassName() + "##" + std::to_string(i);
    bool isSelected = (editorMode->GetSelectedActor() == actor);

    if (ImGui::Selectable(label.c_str(), isSelected)) {
      editorMode->SetSelectedActor(actor);
    }
  }

  ImGui::End();
}

void EditorUI::DrawInspector(EditorMode* editorMode) {
  ImGui::Begin("Inspector");

  AActor* selectedActor = editorMode->GetSelectedActor();
  if (selectedActor && !selectedActor->IsPendingDestroy()) {
    ImGui::Text("Class: %s", selectedActor->GetActorClassName().c_str());
    ImGui::Separator();

    // Transform の編集
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      FVector2D loc = selectedActor->GetActorLocation();
      float locArr[2] = {loc.X, loc.Y};
      if (ImGui::DragFloat2("Location", locArr, 1.0f)) {
        selectedActor->SetActorLocation(FVector2D{locArr[0], locArr[1]});
      }

      FRotator rot = selectedActor->GetActorRotation();
      float rotVal = rot.Rotation;
      if (ImGui::DragFloat("Rotation", &rotVal, 1.0f)) {
        selectedActor->SetActorRotation(FRotator(rotVal));
      }

      FScale scale = selectedActor->GetActorScale();
      float scaleVal = scale.Scale;
      if (ImGui::DragFloat("Scale", &scaleVal, 0.01f)) {
        selectedActor->SetActorScale(FScale(scaleVal));
      }
    }

    // --- SpriteActor専用プロパティ ---
    if (auto spriteActor = dynamic_cast<ASpriteActor*>(selectedActor)) {
      ImGui::Separator();
      if (ImGui::CollapsingHeader("Sprite Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        char pathBuf[512] = {0};
        snprintf(pathBuf, sizeof(pathBuf), "%s", spriteActor->GetImagePath().c_str());
        if (ImGui::InputText("Image Path", pathBuf, sizeof(pathBuf))) {
          spriteActor->SetImagePath(pathBuf);
        }

        if (ImGui::Button("Select Image...")) {
          // ファイル選択ダイアログを開く
          std::string filepath = FileDialog::OpenFile(
              "Image Files (*.png;*.jpg;*.bmp)\0*.png;*.jpg;*.bmp\0All Files (*.*)\0*.*\0"
          );
          if (!filepath.empty()) {
            spriteActor->SetImagePath(filepath);
          }
        }
      }
    }

    ImGui::Separator();

    // アクタの削除
    if (ImGui::Button("Destroy Actor", ImVec2(-1, 0))) {
      selectedActor->Destroy();
      editorMode->SetSelectedActor(nullptr);
    }
  } else {
    ImGui::Text("Select an actor in Outliner to view properties.");
  }

  ImGui::End();
}

void EditorUI::DrawWorldSettings(EditorMode* editorMode) {
  ImGui::Begin("World Settings");

  const auto& gameModeClasses = editorMode->GetGameModeClassList();
  const std::string& selectedGameMode = editorMode->GetSelectedGameModeClass();
  const char* preview = selectedGameMode.empty() ? "(None)" : selectedGameMode.c_str();

  if (ImGui::BeginCombo("GameMode", preview)) {
    for (const auto& className : gameModeClasses) {
      bool isSelected = (className == selectedGameMode);
      if (ImGui::Selectable(className.c_str(), isSelected)) {
        editorMode->SetSelectedGameModeClass(className);
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::End();
}

void EditorUI::DrawSelectAction(EditorMode* editorMode) {
  ImGui::Begin("Action");
  int currentSelection = static_cast<int>(editorMode->GetActorAction());
  ImGui::SameLine();

  if (ImGui::RadioButton("Select", &currentSelection, 0)) {
    editorMode->SetActorAction(EActorAction::Select);
  }
  ImGui::SameLine();

  if (ImGui::RadioButton("Move", &currentSelection, 1)) {
    editorMode->SetActorAction(EActorAction::Move);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", &currentSelection, 2)) {
    editorMode->SetActorAction(EActorAction::Rotate);
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", &currentSelection, 3)) {
    editorMode->SetActorAction(EActorAction::Scale);
  }
  ImGui::End();
}
