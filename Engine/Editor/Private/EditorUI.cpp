#include "EditorUI.h"

#include <imgui.h>

#include "Actor.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "ObjectManager.h"
#include "SpriteActor.h"
#include "UMath.h"
#include "World.h"
void EditorUI::UpdateAndDraw(EditorMode* editorMode) {
  DrawMenuBar(editorMode);

  DrawClassBrowser(editorMode);
  DrawOutliner(editorMode);
  DrawInspector(editorMode);
  DrawWorldSettings(editorMode);

  DrawSelectAction(editorMode);
}

void EditorUI::DrawMenuBar(EditorMode* editorMode) {
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
      if (ImGui::MenuItem("Simulate")) {
        editorMode->Simulate();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void EditorUI::DrawClassBrowser(EditorMode* editorMode) {
  ImGui::Begin("Class Browser");

  const auto& classes = editorMode->GetClassList();
  const std::string& selectedClass = editorMode->GetSelectedClass();

  for (const auto& className : classes) {
    bool isSelected = (className == selectedClass);
    if (ImGui::Selectable(className.c_str(), isSelected)) {
      editorMode->SelectClass(className);
    }
  }

  ImGui::End();
}

void EditorUI::DrawOutliner(EditorMode* editorMode) {
  ImGui::Begin("Outliner");

  const auto& actors = editorMode->GetWorld()->GetObjectManager()->GetAllActors();
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
