#include "Panels/OutlinerPanel.h"

#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

#include "Actor.h"
#include "ActorManager.h"
#include "EditorContext.h"
#include "EditorMode.h"
#include "World.h"

void OutlinerPanel::DrawContents(EditorContext& Context) {
  EditorMode* Mode = Context.Mode;
  const std::vector<std::unique_ptr<AActor>>& Actors =
      Mode->GetWorld()->GetActorManager()->GetAllActors();
  for (size_t Index = 0; Index < Actors.size(); ++Index) {
    AActor* Actor = Actors[Index].get();
    if (Actor == nullptr || Actor->IsPendingDestroy()) continue;

    const std::string Label = Actor->GetInstanceName() + "##" + std::to_string(Index);
    const bool IsSelected = Mode->GetSelectedActor() == Actor;
    if (ImGui::Selectable(Label.c_str(), IsSelected)) {
      Mode->SetSelectedActor(Actor);
    }
  }
}
