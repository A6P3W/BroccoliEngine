#include "Panels/ViewportPanel.h"

#include <imgui.h>

#include "BroccoliRaylib.h"
#include "EditorContext.h"
#include "EditorViewportState.h"
#include "EngineDefine.h"

void ViewportPanel::DrawContents(EditorContext& Context) {
  FEditorViewportState* Viewport = Context.Viewport;
  if (Viewport == nullptr || Viewport->RenderTexture == nullptr) {
    ImGui::TextDisabled("Viewport render texture is unavailable.");
    return;
  }

  auto* RenderTexture = static_cast<RenderTexture2D*>(Viewport->RenderTexture);
  const ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
  if (AvailableSize.x <= 0.0f || AvailableSize.y <= 0.0f) return;

  constexpr float AspectRatio = static_cast<float>(VirtualWidth) / VirtualHeight;
  float DisplayWidth = AvailableSize.x;
  float DisplayHeight = DisplayWidth / AspectRatio;
  if (DisplayHeight > AvailableSize.y) {
    DisplayHeight = AvailableSize.y;
    DisplayWidth = DisplayHeight * AspectRatio;
  }

  const ImVec2 CursorPosition = ImGui::GetCursorPos();
  ImGui::SetCursorPos({
      CursorPosition.x + (AvailableSize.x - DisplayWidth) * 0.5f,
      CursorPosition.y + (AvailableSize.y - DisplayHeight) * 0.5f,
  });
  ImGui::Image(
      ImTextureID(RenderTexture->texture.id),
      {DisplayWidth, DisplayHeight},
      {0.0f, 1.0f},
      {1.0f, 0.0f}
  );

  const ImVec2 ImageMinimum = ImGui::GetItemRectMin();
  const ImVec2 ImageSize = ImGui::GetItemRectSize();
  Viewport->ImagePosition = {ImageMinimum.x, ImageMinimum.y};
  Viewport->ImageSize = {ImageSize.x, ImageSize.y};
  Viewport->Hovered = ImGui::IsItemHovered();
  Viewport->Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}
