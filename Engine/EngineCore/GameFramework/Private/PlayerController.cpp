#include "PlayerController.h"
#include <InputMapper.h>
#include <InputManager.h>
#include <EnhancedInputComponent.h>
#include <DxLib.h>
#include <InputDevice.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <GamePadDevice.h>
#include <UIManager.h>

REGISTER_ACTOR(APlayerController)


APlayerController::APlayerController()
{
	auto InputComp = std::make_unique<MEnhancedInputComponent>();
	InputComponent = InputComp.get();
	AddComponent(std::move(InputComp));
	InputMapperPtr = std::make_unique<InputMapper>();
	SetUpdateableAnytime(true);
	SetPlayerId(0);
}

void APlayerController::Possess(APawn* NewPawn)
{
	if (GetInputComponent()) {
		GetInputComponent()->ClearBindings();
	}

	if (TargetPawn && TargetPawn != NewPawn) {
		TargetPawn->OnUnPossessed();
	}

	TargetPawn = NewPawn;

	if (TargetPawn) {
		TargetPawn->OnPossessedBy(this);
		
		SetupPlayerInputComponent(GetInputComponent());
		if (GetInputComponent()) {
			TargetPawn->SetupPlayerInputComponent(GetInputComponent());
		}
	}
}

void APlayerController::OnUpdate(float DeltaTime)
{
	if (!TargetPawn)return;
	if (MEnhancedInputComponent* InputComp = GetInputComponent()) {
		bool bAllowUI = (InputMode == EInputMode::UIOnly || InputMode == EInputMode::GameAndUI);
		bool bAllowGame = (InputMode == EInputMode::GameOnly || InputMode == EInputMode::GameAndUI);
		InputComp->ProcessInputBindings(*InputMapperPtr, bAllowUI, bAllowGame);
	}
}

void APlayerController::SetPlayerId(int id)
{
	PlayerId = id;
}

void APlayerController::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{
	auto* manager = UIManager::GetInstance();
	PlayerInputComponent->BindAction(UIAction::Move, ETriggerEvent::Triggered, manager, &UIManager::Navigate, true);
	PlayerInputComponent->BindAction(UIAction::Submit, ETriggerEvent::Started, manager, &UIManager::Submit, true);
	PlayerInputComponent->BindAction(UIAction::Cancel, ETriggerEvent::Started, manager, &UIManager::Cancel, true);
}

void APlayerController::SetupInputMappings()
{
	InputMapperPtr->RemoveMapping(InputActionLower::MoveX);
	InputMapperPtr->RemoveMapping(InputActionLower::MoveY);
	InputMapperPtr->RemoveMapping(UIActionLower::MoveX);
	InputMapperPtr->RemoveMapping(UIActionLower::MoveY);
	InputMapperPtr->RemoveMapping(InputActionMouse::Wheel);
	InputMapperPtr->RemoveMapping(InputAction::Interact);

	auto& IM = InputManager::GetInstance();
	auto* kb = IM.GetDevice<KeyboardDevice>();
	auto* mouse = IM.GetDevice<MouseDevice>();
	auto* pad = IM.GetDevice<GamepadDevice>();

	if (kb) {
		// ゲーム移動
		InputMapperPtr->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_A, "", 1.0f);
		InputMapperPtr->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_D, "", -1.0f);
		InputMapperPtr->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_W, "", 1.0f);
		InputMapperPtr->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_S, "", -1.0f);

		// UI操作
		InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, KEY_INPUT_A, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, KEY_INPUT_D, "", -1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, KEY_INPUT_W, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, KEY_INPUT_S, "", -1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, KEY_INPUT_UP, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, KEY_INPUT_DOWN, "", -1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, KEY_INPUT_LEFT, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, KEY_INPUT_RIGHT, "", -1.0f);

		InputMapperPtr->AddMapping(InputAction::Interact, kb, KEY_INPUT_F);
		InputMapperPtr->AddMapping(UIAction::Submit, kb, KEY_INPUT_SPACE);
		InputMapperPtr->AddMapping(UIAction::Cancel, kb, KEY_INPUT_ESCAPE);
		InputMapperPtr->AddMapping(InputAction::Pause, kb, KEY_INPUT_ESCAPE);
	}
	if (mouse) {
		InputMapperPtr->AddMapping(InputActionMouse::MouseLeft, mouse, MOUSE_INPUT_LEFT);
		InputMapperPtr->AddMapping(InputActionMouse::MouseRight, mouse, MOUSE_INPUT_RIGHT);
		InputMapperPtr->AddAxisMapping(InputActionMouse::Wheel, mouse, MouseDevice::AxisID::Wheel);
		InputMapperPtr->AddAxisMapping(InputActionLower::LookX, mouse, MouseDevice::AxisID::MouseX);
		InputMapperPtr->AddAxisMapping(InputActionLower::LookY, mouse, MouseDevice::AxisID::MouseY);
	}
	if (pad) {
		// ゲーム移動
		InputMapperPtr->AddAxisMapping(InputActionLower::MoveX, pad, static_cast<int>(AxisID::LeftX), -1.0f);
		InputMapperPtr->AddAxisMapping(InputActionLower::MoveY, pad, static_cast<int>(AxisID::LeftY), 1.0f);

		// UI操作
		InputMapperPtr->AddAxisMapping(UIActionLower::MoveX, pad, static_cast<int>(AxisID::LeftX), -1.0f);
		InputMapperPtr->AddAxisMapping(UIActionLower::MoveY, pad, static_cast<int>(AxisID::LeftY), 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, pad, PAD_INPUT_UP, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveY, pad, PAD_INPUT_DOWN, "", -1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveX, pad, PAD_INPUT_LEFT, "", 1.0f);
		InputMapperPtr->AddMapping(UIActionLower::MoveX, pad, PAD_INPUT_RIGHT, "", -1.0f);

		InputMapperPtr->AddMapping(InputAction::Interact, pad, PAD_INPUT_1);
		InputMapperPtr->AddMapping(UIAction::Submit, pad, PAD_INPUT_1);
		InputMapperPtr->AddMapping(UIAction::Cancel, pad, PAD_INPUT_2);
		InputMapperPtr->AddMapping(InputAction::Pause, pad, PAD_INPUT_8);
	}
}
