#include "Panels/ViewportPanel.h"

#include <imgui.h>

#include "BroccoliRaylib.h"
#include "EditorContext.h"
#include "EditorViewportState.h"
#include "RenderSystem.h"

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

  ImGui::Image(ImTextureID(RenderTexture->texture.id), AvailableSize, {0.0f, 1.0f}, {1.0f, 0.0f});

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
