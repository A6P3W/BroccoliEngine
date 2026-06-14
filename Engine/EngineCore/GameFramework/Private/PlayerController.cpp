#include "PlayerController.h"
#include <InputMapper.h>
#include <InputManager.h>
#include <EnhancedInputComponent.h>
#include <DxLib.h>
#include <InputDevice.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <UIManager.h>
APlayerController::APlayerController()
{
    auto InputComp = std::make_unique<MEnhancedInputComponent>();
    m_InputCompPtr = InputComp.get();
    AddComponent(std::move(InputComp));
    m_InputMapper = std::make_unique<InputMapper>();


    SetPlayerId(0);
}

void APlayerController::Possess(APawn* NewPawn)
{
    if (GetInputComponent()) {
        GetInputComponent()->ClearBindings();
    }
	m_TargetPawn = NewPawn;
    m_TargetPawn->OnPossesed();

	SetupPlayerInputComponent(GetInputComponent());

    if (GetInputComponent()) {
        m_TargetPawn->SetupPlayerInputComponent(GetInputComponent());
    }
}

void APlayerController::OnUpdate(float DeltaTime)
{
	if (!m_TargetPawn)return;

    if (MEnhancedInputComponent* InputComp = GetInputComponent()) {
		bool bAllowUI = (InputMode == EInputMode::UIOnly || InputMode == EInputMode::GameAndUI);
		bool bAllowGame = (InputMode == EInputMode::GameOnly || InputMode == EInputMode::GameAndUI);
        InputComp->ProcessInputBindings(*m_InputMapper, bAllowUI, bAllowGame);
    }
}

void APlayerController::SetPlayerId(int id)
{
    m_PlayerId = id;
}

void APlayerController::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{
	auto* manager=UIManager::GetInstance();
    PlayerInputComponent->BindAction(InputAction::Move, ETriggerEvent::Triggered, manager, &UIManager::Navigate, true);

    PlayerInputComponent->BindAction(UIAction::Submit, ETriggerEvent::Started, manager, &UIManager::Submit, true);
	PlayerInputComponent->BindAction(UIAction::Cancel, ETriggerEvent::Started, manager,&UIManager::Cancel,true );
}

void APlayerController::SetupInputMappings()
{
    m_InputMapper->RemoveMapping(InputActionLower::MoveX);
    m_InputMapper->RemoveMapping(InputActionLower::MoveY);
    m_InputMapper->RemoveMapping(InputActionMouse::Wheel);
    m_InputMapper->RemoveMapping(InputAction::Interact);

    auto& IM = InputManager::GetInstance();
    auto* kb = IM.GetDevice<KeyboardDevice>();
    auto* mouse = IM.GetDevice<MouseDevice>();

    if (kb) {
        // キーボード
        m_InputMapper->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_A, "", 1.0f);
        m_InputMapper->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_D, "", -1.0f);
        m_InputMapper->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_W, "", 1.0f);
        m_InputMapper->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_S, "", -1.0f);
        m_InputMapper->AddMapping(InputAction::Interact, kb, KEY_INPUT_F);

        m_InputMapper->AddMapping(UIAction::Submit, kb, KEY_INPUT_SPACE);
		m_InputMapper->AddMapping(UIAction::Cancel, kb, KEY_INPUT_ESCAPE);
    }
    if (mouse) {
        m_InputMapper->AddMapping(InputActionMouse::MouseLeft, mouse, MOUSE_INPUT_LEFT);
        m_InputMapper->AddMapping(InputActionMouse::MouseRight, mouse, MOUSE_INPUT_RIGHT);

        m_InputMapper->AddAxisMapping(InputActionMouse::Wheel, mouse, MouseDevice::AxisID::Wheel);

        m_InputMapper->AddAxisMapping(InputActionLower::LookX, mouse, MouseDevice::AxisID::MouseX);
        m_InputMapper->AddAxisMapping(InputActionLower::LookY, mouse, MouseDevice::AxisID::MouseY);
    }
}
