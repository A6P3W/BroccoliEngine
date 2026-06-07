#include "EditorController.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "KeyboardDevice.h"
#include "EnhancedInputComponent.h"
#include "EditorMode.h"
#include "World.h"
#include <DxLib.h>

EditorController::EditorController()
{
}

void EditorController::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAction(EditorInputAction::SelectMode, ETriggerEvent::Started, this, &EditorController::OnSelectModePressed);
	PlayerInputComponent->BindAction(EditorInputAction::MoveMode, ETriggerEvent::Started, this, &EditorController::OnMoveModePressed);
	PlayerInputComponent->BindAction(EditorInputAction::RotateMode, ETriggerEvent::Started, this, &EditorController::OnRotateModePressed);
	PlayerInputComponent->BindAction(EditorInputAction::ScaleMode, ETriggerEvent::Started, this, &EditorController::OnScaleModePressed);
}

void EditorController::SetPlayerId(int id)
{
	EditorModePtr = dynamic_cast<EditorMode*>(GetWorld()->GetGameMode());

	APlayerController::SetPlayerId(id);
	auto& IM = InputManager::GetInstance();
	auto* kb = IM.GetDevice<KeyboardDevice>();

	auto*Mapper = GetInputMapper();

	if (id == 0 && kb) {
		Mapper->AddMapping(EditorInputAction::SelectMode, kb, KEY_INPUT_Q);
		Mapper->AddMapping(EditorInputAction::MoveMode, kb, KEY_INPUT_W);
		Mapper->AddMapping(EditorInputAction::RotateMode, kb, KEY_INPUT_E);
		Mapper->AddMapping(EditorInputAction::ScaleMode, kb, KEY_INPUT_R);

	}

}

void EditorController::OnSelectModePressed() {
	EditorModePtr->SetActorAction(EActorAction::Select);
	M_LOG("EditorMode: Select");
}

void EditorController::OnMoveModePressed() {
	EditorModePtr->SetActorAction(EActorAction::Move);
	M_LOG("EditorMode: Move");
}

void EditorController::OnRotateModePressed() {
	EditorModePtr->SetActorAction(EActorAction::Rotate);
	M_LOG("EditorMode: Rotate");
}

void EditorController::OnScaleModePressed() {
	EditorModePtr->SetActorAction(EActorAction::Scale);
	M_LOG("EditorMode: Scale");
}
