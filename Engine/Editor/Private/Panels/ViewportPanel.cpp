#include "Panels/ViewportPanel.h"

#include <imgui.h>

#include "ActorRegistry.h"
#include "BroccoliRaylib.h"
#include "Camera3DComponent.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "EditorPawn.h"
#include "EditorViewportState.h"
#include "FileDialog.h"
#include "PathResolver.h"
#include "RenderSystem.h"
#include "StaticMeshActor.h"

void ViewportPanel::DrawContents(EditorContext& Context) {
  FEditorViewportState* Viewport = Context.Viewport;
  if (Viewport == nullptr || Viewport->RenderTexture == nullptr) {
    ImGui::TextDisabled("Viewport render texture is unavailable.");
    return;
  }

  auto* RenderTexture = static_cast<RenderTexture2D*>(Viewport->RenderTexture);
  const ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
  if (AvailableSize.x <= 0.0f || AvailableSize.y <= 0.0f) return;
  Viewport->RequestedRenderSize = {AvailableSize.x, AvailableSize.y};

  const bool IsThreeDMode = Viewport->Mode == EEditorViewportMode::ThreeD;
  if (ImGui::BeginChild("ViewportToolbar", ImVec2(0.0f, 28.0f), false)) {
    if (ImGui::RadioButton("2D", !IsThreeDMode))
      Context.Mode->SetViewportMode(EEditorViewportMode::TwoD);
    ImGui::SameLine();
    if (ImGui::RadioButton("3D", IsThreeDMode))
      Context.Mode->SetViewportMode(EEditorViewportMode::ThreeD);
    if (IsThreeDMode) {
      ImGui::SameLine();
      if (ImGui::Button("Add Static Mesh...")) {
        const std::string FilePath = FileDialog::OpenFile(
            "3D Model Files (*.glb;*.gltf)\0*.glb;*.gltf\0All Files (*.*)\0*.*\0",
            PathResolver::GetGameResourceDir()
        );
        CreateStaticMeshActor(Context, FilePath);
      }
      ImGui::SameLine();
      ImGui::TextDisabled("RMB Look | WASD Move | Q/E Vertical | F Focus");
    }
  }
  ImGui::EndChild();

  const ImVec2 ImageAvailableSize = ImGui::GetContentRegionAvail();
  if (ImageAvailableSize.x <= 0.0f || ImageAvailableSize.y <= 0.0f) return;
  Viewport->RequestedRenderSize = {ImageAvailableSize.x, ImageAvailableSize.y};

  ImGui::Image(
      ImTextureID(RenderTexture->texture.id), ImageAvailableSize, {0.0f, 1.0f}, {1.0f, 0.0f}
  );

  const ImVec2 ImageMinimum = ImGui::GetItemRectMin();
  const ImVec2 ImageSize = ImGui::GetItemRectSize();
  Viewport->ImagePosition = {ImageMinimum.x, ImageMinimum.y};
  Viewport->ImageSize = {ImageSize.x, ImageSize.y};
  Viewport->Hovered = ImGui::IsItemHovered();
  Viewport->Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

  if (Viewport->RenderTargetSize.X <= 0.0f || Viewport->RenderTargetSize.Y <= 0.0f) return;

  const FScreenRenderArea SafeArea = RenderSystem::GetInstance().GetScreenRenderArea();
  const ImVec2 SafeAreaMinimum = {
      ImageMinimum.x + SafeArea.Position.X * ImageSize.x / Viewport->RenderTargetSize.X,
      ImageMinimum.y + SafeArea.Position.Y * ImageSize.y / Viewport->RenderTargetSize.Y
  };
  const ImVec2 SafeAreaMaximum = {
      SafeAreaMinimum.x + SafeArea.Size.X * ImageSize.x / Viewport->RenderTargetSize.X,
      SafeAreaMinimum.y + SafeArea.Size.Y * ImageSize.y / Viewport->RenderTargetSize.Y
  };
  ImGui::GetWindowDrawList()->AddRect(
      SafeAreaMinimum, SafeAreaMaximum, IM_COL32(255, 196, 0, 200), 0.0f, 0, 1.0f
  );
}

void ViewportPanel::CreateStaticMeshActor(
    EditorContext& Context, const std::string& ModelPath
) const {
  if (ModelPath.empty()) return;

  AActor* Actor = ActorRegistry::GetInstance().Spawn(
      Context.Mode->GetWorld(), AStaticMeshActor::StaticClassName()
  );
  auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(Actor);
  if (StaticMeshActor == nullptr) return;

  EditorPawn* EditorPawn = Context.Mode->GetEditorPawn();
  if (EditorPawn != nullptr && EditorPawn->GetEditorCamera3D() != nullptr) {
    StaticMeshActor->SetActorLocation3D(
        EditorPawn->GetActorLocation3D() +
        EditorPawn->GetEditorCamera3D()->GetForwardVector() * 5.0f
    );
  }
  StaticMeshActor->SetModelPath(ModelPath);
  Context.Mode->SetSelectedActor(StaticMeshActor);
}
