#include "Panels/InspectorPanel.h"

#include <imgui.h>

#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "Actor.h"
#include "ComponentRegistry.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "FileDialog.h"
#include "Log.h"
#include "PathResolver.h"
#include "Reflection.h"
#include "UMath.h"

namespace {
std::unordered_map<ImGuiID, std::vector<char>> PathBuffers;
AActor* PathBufferActor = nullptr;

void ResetPathBuffers(AActor* Actor) {
  if (PathBufferActor == Actor) return;
  PathBuffers.clear();
  PathBufferActor = Actor;
}

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

void DrawReflectedProperty(void* Object, const FProperty& Property) {
  FPropertyValue Value = Property.Get(Object);
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
    case EPropertyType::Path: {
      const std::string& Current = std::get<FPath>(Value).String();
      ImGui::PushID(Object);
      ImGui::PushID(Label);
      ImGui::TextUnformatted(Label);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(-90.0f);
      const ImGuiID InputId = ImGui::GetID("##Path");
      auto& Buffer = PathBuffers[InputId];
      if (Buffer.empty()) {
        Buffer.resize(std::max<std::size_t>(Current.size() + 1, 64), '\0');
        std::memcpy(Buffer.data(), Current.c_str(), Current.size());
      }
      ImGui::InputText(
          "##Path",
          Buffer.data(),
          Buffer.size(),
          ImGuiInputTextFlags_CallbackResize,
          ResizeStringBuffer,
          &Buffer
      );
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        try {
          Value = FPath(std::string(Buffer.data()));
          Changed = true;
        } catch (const std::exception&) {
          M_LOG(Warning, "Inspector rejected invalid path for '{}'.", Property.Name);
        }
        Buffer.clear();
      } else if (!ImGui::IsItemActive()) {
        Buffer.clear();
      }
      ImGui::SameLine();
      if (ImGui::Button("Select...")) {
        const char* Filter = "All Files (*.*)\0*.*\0";
        if (Property.EditorMetadata.PathFilter == EPathFilter::Image)
          Filter =
              "Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files "
              "(*.*)\0*.*\0";
        else if (Property.EditorMetadata.PathFilter == EPathFilter::Model)
          Filter = "3D Model Files (*.glb;*.gltf)\0*.glb;*.gltf\0All Files (*.*)\0*.*\0";
        const std::string Directory = Current.starts_with("/Engine/")
                                          ? PathResolver::GetEngineResourceDir()
                                          : PathResolver::GetGameResourceDir();
        const std::string Selected = FileDialog::OpenFile(Filter, Directory);
        if (!Selected.empty()) {
          try {
            Value = FPath(Selected);
            Changed = true;
          } catch (const std::exception&) {
            M_LOG(Warning, "Selected file is outside the resource roots: {}", Selected);
          }
        }
      }
      ImGui::PopID();
      ImGui::PopID();
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
  if (Changed && !Property.Set(Object, Value)) {
    M_LOG(Warning, "Inspector rejected reflected property '{}'.", Property.Name);
    Value = Property.Get(Object);
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
    ResetPathBuffers(nullptr);
    ImGui::Text("Select an actor in Outliner to view properties.");
    return;
  }
  ResetPathBuffers(SelectedActor);

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

  if (const FClass* Class =
          FReflectionRegistry::GetInstance().FindClass(SelectedActor->GetActorClassName())) {
    const auto Properties = Class->GetProperties();
    if (!Properties.empty()) {
      ImGui::Separator();
      if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const FProperty* Property : Properties) {
          if (Property->Name == "Location" || Property->Name == "Rotation" ||
              Property->Name == "Scale")
            continue;
          DrawReflectedProperty(SelectedActor, *Property);
        }
      }
    }
  }

  ImGui::Separator();
  if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const auto& Owned : SelectedActor->GetComponents()) {
      MActorComponent* Component = Owned.get();
      if (!Component || Component->IsPendingDestroy()) continue;
      ImGui::PushID(Component);
      const std::string Label =
          Component->GetComponentName() + " (" + Component->GetComponentClassName() + ")";
      if (ImGui::CollapsingHeader(Label.c_str())) {
        if (const FClass* Class =
                FReflectionRegistry::GetInstance().FindClass(Component->GetComponentClassName()))
          for (const FProperty* Property : Class->GetProperties())
            DrawReflectedProperty(Component, *Property);
        if (Component->GetCreationSource() == EComponentCreationSource::Instance &&
            Component != SelectedActor->GetRootComponent() && ImGui::Button("Remove Component"))
          Component->DestroyComponent();
      }
      ImGui::PopID();
    }
  }

  if (ImGui::BeginCombo("Add Component", "Select class")) {
    for (const FComponentClassInfo& Info : ComponentRegistry::GetInstance().GetClasses()) {
      if (!Info.Options.EditorAddable) continue;
      bool CanAdd = true;
      if (!Info.Options.AllowMultiple)
        for (const auto& Existing : SelectedActor->GetComponents())
          if (Existing && !Existing->IsPendingDestroy() &&
              Existing->GetComponentClassName() == Info.ClassName)
            CanAdd = false;
      if (CanAdd && ImGui::Selectable(Info.ClassName.c_str())) {
        if (MActorComponent* Component =
                ComponentRegistry::GetInstance().Create(SelectedActor, Info.ClassName))
          Component->RegisterComponent();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::Separator();
  if (ImGui::Button("Destroy Actor", ImVec2(-1.0f, 0.0f))) {
    SelectedActor->Destroy();
    Mode->SetSelectedActor(nullptr);
  }
}
