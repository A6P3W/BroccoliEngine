#include "Panels/InspectorPanel.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "Actor.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "Log.h"
#include "PathResolver.h"
#include "Reflection.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "UMath.h"

namespace {
bool IsSameRotation(const FQuaternion& Left, const FQuaternion& Right) {
  const FQuaternion NormalizedLeft = Left.Normalize();
  const FQuaternion NormalizedRight = Right.Normalize();
  const float Dot = NormalizedLeft.X * NormalizedRight.X + NormalizedLeft.Y * NormalizedRight.Y +
                    NormalizedLeft.Z * NormalizedRight.Z + NormalizedLeft.W * NormalizedRight.W;
  return std::abs(Dot) > 0.999999f;
}

int ResizeStringBuffer(ImGuiInputTextCallbackData* Data) {
  auto* Buffer = static_cast<std::vector<char>*>(Data->UserData);
  Buffer->resize(static_cast<std::size_t>(Data->BufSize) * 2);
  Data->Buf = Buffer->data();
  return 0;
}

void DrawReflectedProperty(AActor* Actor, const FProperty& Property) {
  FPropertyValue Value = Property.Get(Actor);
  const char* Label = Property.Name.c_str();
  bool Changed = false;
  switch (Property.Type) {
    case EPropertyType::Bool: {
      bool Edited = std::get<bool>(Value);
      Changed = ImGui::Checkbox(Label, &Edited);
      if (Changed) Value = Edited;
      break;
    }
    case EPropertyType::Int: {
      int Edited = std::get<int>(Value);
      const int Min = Property.EditorMetadata.IntSliderMin.value_or(
          Property.EditorMetadata.IntMin.value_or(std::numeric_limits<int>::min())
      );
      const int Max = Property.EditorMetadata.IntSliderMax.value_or(
          Property.EditorMetadata.IntMax.value_or(std::numeric_limits<int>::max())
      );
      Changed = ImGui::DragInt(Label, &Edited, 1.0f, Min, Max);
      if (Changed) Value = Edited;
      break;
    }
    case EPropertyType::Float: {
      float Edited = std::get<float>(Value);
      const float Min = Property.EditorMetadata.FloatSliderMin.value_or(
          Property.EditorMetadata.FloatMin.value_or(-std::numeric_limits<float>::max())
      );
      const float Max = Property.EditorMetadata.FloatSliderMax.value_or(
          Property.EditorMetadata.FloatMax.value_or(std::numeric_limits<float>::max())
      );
      Changed = ImGui::DragFloat(Label, &Edited, 0.1f, Min, Max);
      if (Changed) Value = Edited;
      break;
    }
    case EPropertyType::String: {
      const std::string& Current = std::get<std::string>(Value);
      std::vector<char> Buffer(std::max<std::size_t>(Current.size() + 1, 64), '\0');
      std::memcpy(Buffer.data(), Current.c_str(), Current.size());
      Changed = ImGui::InputText(
          Label,
          Buffer.data(),
          Buffer.size(),
          ImGuiInputTextFlags_CallbackResize,
          ResizeStringBuffer,
          &Buffer
      );
      if (Changed) Value = std::string(Buffer.data());
      break;
    }
    case EPropertyType::Vector2D: {
      const auto& Current = std::get<FVector2D>(Value);
      float Edited[2] = {Current.X, Current.Y};
      Changed = ImGui::DragFloat2(Label, Edited, 0.1f);
      if (Changed) Value = FVector2D{Edited[0], Edited[1]};
      break;
    }
    case EPropertyType::Vector3D: {
      const auto& Current = std::get<FVector3D>(Value);
      float Edited[3] = {Current.X, Current.Y, Current.Z};
      Changed = ImGui::DragFloat3(Label, Edited, 0.1f);
      if (Changed) Value = FVector3D{Edited[0], Edited[1], Edited[2]};
      break;
    }
  }
  if (Changed && !Property.Set(Actor, Value)) {
    M_LOG(Warning, "Inspector rejected reflected property '{}'.", Property.Name);
    Value = Property.Get(Actor);
  }
}
}  // namespace

void InspectorPanel::SynchronizeRotation(AActor* Actor) {
  const FQuaternion ActorRotation = Actor->GetActorRotation3D();
  if (RotationActor == Actor && bHasCachedRotation &&
      IsSameRotation(ActorRotation, LastAppliedRotation)) {
    return;
  }

  RotationActor = Actor;
  CachedRotation = ActorRotation.ToRotator();
  LastAppliedRotation = ActorRotation;
  bHasCachedRotation = true;
}

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

      SynchronizeRotation(SelectedActor);
      float RotationValues[3] = {CachedRotation.Pitch, CachedRotation.Yaw, CachedRotation.Roll};
      if (ImGui::DragFloat3("Rotation", RotationValues, 1.0f)) {
        CachedRotation = {RotationValues[0], RotationValues[1], RotationValues[2]};
        LastAppliedRotation = FQuaternion::FromRotator(CachedRotation);
        SelectedActor->SetActorRotation3D(LastAppliedRotation);
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

  if (const FClass* Class =
          FReflectionRegistry::GetInstance().FindClass(SelectedActor->GetActorClassName())) {
    const auto Properties = Class->GetProperties();
    if (!Properties.empty()) {
      ImGui::Separator();
      if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const FProperty* Property : Properties) {
          if (Property->Name == "ImagePath" || Property->Name == "ModelPath" ||
              Property->Name == "Location" || Property->Name == "Rotation" ||
              Property->Name == "Scale")
            continue;
          DrawReflectedProperty(SelectedActor, *Property);
        }
      }
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Destroy Actor", ImVec2(-1.0f, 0.0f))) {
    SelectedActor->Destroy();
    Mode->SetSelectedActor(nullptr);
  }
}
