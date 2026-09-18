#include "Panels/InspectorPanel.h"

#include <imgui.h>

#include <cstdio>
#include <string>

#include "Actor.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "PathResolver.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "UMath.h"

void InspectorPanel::DrawContents(EditorContext& Context) {
  EditorMode* Mode = Context.Mode;
  AActor* SelectedActor = Mode->GetSelectedActor();
  if (SelectedActor == nullptr || SelectedActor->IsPendingDestroy()) {
    ImGui::Text("Select an actor in Outliner to view properties.");
    return;
  }

  ImGui::Text("Class: %s", SelectedActor->GetActorClassName().c_str());
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (Context.Viewport->Mode == EEditorViewportMode::ThreeD) {
      const FVector3D Location = SelectedActor->GetActorLocation3D();
      float LocationValues[3] = {Location.X, Location.Y, Location.Z};
      if (ImGui::DragFloat3("Location", LocationValues, 0.1f)) {
        SelectedActor->SetActorLocation3D(
            {LocationValues[0], LocationValues[1], LocationValues[2]}
        );
      }

      const FRotator3D Rotation = SelectedActor->GetActorRotation3D().ToRotator();
      float RotationValues[3] = {Rotation.Pitch, Rotation.Yaw, Rotation.Roll};
      if (ImGui::DragFloat3("Rotation", RotationValues, 1.0f)) {
        SelectedActor->SetActorRotation3D(
            FQuaternion::FromRotator({RotationValues[0], RotationValues[1], RotationValues[2]})
        );
      }

      const FScale3D Scale = SelectedActor->GetActorScale3D();
      float ScaleValues[3] = {Scale.X, Scale.Y, Scale.Z};
      if (ImGui::DragFloat3("Scale", ScaleValues, 0.01f)) {
        SelectedActor->SetActorScale3D({ScaleValues[0], ScaleValues[1], ScaleValues[2]});
      }
    } else {
      const FVector2D Location = SelectedActor->GetActorLocation();
      float LocationValues[2] = {Location.X, Location.Y};
      if (ImGui::DragFloat2("Location", LocationValues, 1.0f)) {
        SelectedActor->SetActorLocation(FVector2D{LocationValues[0], LocationValues[1]});
      }

      const FRotator Rotation = SelectedActor->GetActorRotation();
      float RotationValue = Rotation.Rotation;
      if (ImGui::DragFloat("Rotation", &RotationValue, 1.0f)) {
        SelectedActor->SetActorRotation(FRotator(RotationValue));
      }

      const FScale Scale = SelectedActor->GetActorScale();
      float ScaleValue = Scale.Scale;
      if (ImGui::DragFloat("Scale", &ScaleValue, 0.01f)) {
        SelectedActor->SetActorScale(FScale(ScaleValue));
      }
    }
  }

  if (auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(SelectedActor)) {
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
      char PathBuffer[512] = {};
      std::snprintf(PathBuffer, sizeof(PathBuffer), "%s", StaticMeshActor->GetModelPath().c_str());
      if (ImGui::InputText("Model Path", PathBuffer, sizeof(PathBuffer))) {
        StaticMeshActor->SetModelPath(PathBuffer);
      }
      if (ImGui::Button("Select Model...")) {
        const std::string FilePath = FileDialog::OpenFile(
            "3D Model Files (*.glb;*.gltf)\0*.glb;*.gltf\0All Files (*.*)\0*.*\0",
            PathResolver::GetGameResourceDir()
        );
        if (!FilePath.empty()) StaticMeshActor->SetModelPath(FilePath);
      }
    }
  }

  if (auto* SpriteActor = dynamic_cast<ASpriteActor*>(SelectedActor)) {
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Sprite Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
      char PathBuffer[512] = {};
      std::snprintf(PathBuffer, sizeof(PathBuffer), "%s", SpriteActor->GetImagePath().c_str());
      if (ImGui::InputText("Image Path", PathBuffer, sizeof(PathBuffer))) {
        SpriteActor->SetImagePath(PathBuffer);
      }

      if (ImGui::Button("Select Image...")) {
        const std::string DialogDirectory = PathResolver::GetGameResourceDir();
        const std::string FilePath = FileDialog::OpenFile(
            "Image Files (*.png;*.jpg;*.bmp)\0*.png;*.jpg;*.bmp\0All Files (*.*)\0*.*\0",
            DialogDirectory
        );
        if (!FilePath.empty()) SpriteActor->SetImagePath(FilePath);
      }
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Destroy Actor", ImVec2(-1.0f, 0.0f))) {
    SelectedActor->Destroy();
    Mode->SetSelectedActor(nullptr);
  }
}
