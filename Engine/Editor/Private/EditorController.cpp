#include "EditorController.h"

#include "EditorMode.h"
#include "EnhancedInputComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "KeyboardDevice.h"
#include "World.h"
REGISTER_ACTOR(EditorController);
EditorController::EditorController() { bEditorActor = true; }

void EditorController::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  APlayerController::SetupPlayerInputComponent(PlayerInputComponent);

  EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());

  PlayerInputComponent->BindAction(
      EditorInputAction::SelectMode,
      ETriggerEvent::Started,
      this,
      &EditorController::OnSelectModePressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::MoveMode,
      ETriggerEvent::Started,
      this,
      &EditorController::OnMoveModePressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::RotateMode,
      ETriggerEvent::Started,
      this,
      &EditorController::OnRotateModePressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::ScaleMode,
      ETriggerEvent::Started,
      this,
      &EditorController::OnScaleModePressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::Copy, ETriggerEvent::Started, this, &EditorController::OnCopyPressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::Paste, ETriggerEvent::Started, this, &EditorController::OnPastePressed
  );
  PlayerInputComponent->BindAction(
      EditorInputAction::Cut, ETriggerEvent::Started, this, &EditorController::OnCutPressed
  );
  PlayerInputComponent->BindAction(
      "Delete", ETriggerEvent::Started, this, &EditorController::OnDeletePressed
  );
  PlayerInputComponent->BindAction(
      "Save", ETriggerEvent::Started, this, &EditorController::OnSavePressed
  );
}

void EditorController::SetupInputMappings() {
  APlayerController::SetupInputMappings();

  auto& IM = InputManager::GetInstance();
  auto* kb = IM.GetDevice<KeyboardDevice>();

  auto* Mapper = GetInputMapper();

  if (kb) {
    Mapper->AddMapping(EditorInputAction::SelectMode, kb, EKey::Q);
    Mapper->AddMapping(EditorInputAction::MoveMode, kb, EKey::W);
    Mapper->AddMapping(EditorInputAction::RotateMode, kb, EKey::E);
    Mapper->AddMapping(EditorInputAction::ScaleMode, kb, EKey::R);

    Mapper->AddMapping(EditorInputAction::ModifierCtrl, kb, EKey::LeftControl);
    Mapper->AddMapping(EditorInputAction::Copy, kb, EKey::C, EditorInputAction::ModifierCtrl);
    Mapper->AddMapping(EditorInputAction::Paste, kb, EKey::V, EditorInputAction::ModifierCtrl);
    Mapper->AddMapping(EditorInputAction::Cut, kb, EKey::X, EditorInputAction::ModifierCtrl);
    Mapper->AddMapping("Save", kb, EKey::S, EditorInputAction::ModifierCtrl);
    Mapper->AddMapping("Delete", kb, EKey::Delete);
  }
}

void EditorController::OnSelectModePressed() {
  EditorModePtr->SetActorAction(EActorAction::Select);
  M_LOG(Log, "EditorMode: Select");
}

void EditorController::OnMoveModePressed() {
  EditorModePtr->SetActorAction(EActorAction::Move);
  M_LOG(Log, "EditorMode: Move");
}

void EditorController::OnRotateModePressed() {
  EditorModePtr->SetActorAction(EActorAction::Rotate);
  M_LOG(Log, "EditorMode: Rotate");
}

void EditorController::OnScaleModePressed() {
  EditorModePtr->SetActorAction(EActorAction::Scale);
  M_LOG(Log, "EditorMode: Scale");
}

void EditorController::OnCopyPressed() {
  if (EditorModePtr) {
    EditorModePtr->CopySelectedActor();
  }
}

void EditorController::OnPastePressed() {
  if (EditorModePtr) {
    EditorModePtr->PasteActor();
  }
}

void EditorController::OnCutPressed() {
  if (EditorModePtr) {
    EditorModePtr->CutSelectedActor();
  }
}

void EditorController::OnDeletePressed() {
  if (EditorModePtr) {
    EditorModePtr->DeleteSelectedActor();
  }
}

void EditorController::OnSavePressed() {
  if (EditorModePtr) {
    EditorModePtr->QuickSaveLevel();
  }
}
