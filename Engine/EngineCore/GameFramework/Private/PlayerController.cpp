#include "PlayerController.h"
#include <InputMapper.h>
#include <InputManager.h>
#include <EnhancedInputComponent.h>
#include <DxLib.h>
#include <InputDevice.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
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

    if (GetInputComponent()) {
        m_TargetPawn->SetupPlayerInputComponent(GetInputComponent());
    }
}

void APlayerController::OnUpdate(float DeltaTime)
{
	if (!m_TargetPawn)return;

    if (MEnhancedInputComponent* InputComp = GetInputComponent()) {
        InputComp->ProcessInputBindings(*m_InputMapper);
    }
}

void APlayerController::SetPlayerId(int id)
{
    m_PlayerId = id;
    m_InputMapper->RemoveMapping(InputActionLower::MoveX);
    m_InputMapper->RemoveMapping(InputActionLower::MoveY);
    m_InputMapper->RemoveMapping(InputActionMouse::Wheel);
    m_InputMapper->RemoveMapping(InputAction::Interact);

    auto& IM = InputManager::GetInstance();
    auto* kb = IM.GetDevice<KeyboardDevice>();
    auto* mouse = IM.GetDevice<MouseDevice>();
    //uto* pad = IM.GetDevice<GamepadDevice>(); 

    if (id == 0 && kb) {
        // キーボード
        m_InputMapper->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_A, 1.0f); 
        m_InputMapper->AddMapping(InputActionLower::MoveX, kb, KEY_INPUT_D, -1.0f);
        m_InputMapper->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_W, 1.0f);
        m_InputMapper->AddMapping(InputActionLower::MoveY, kb, KEY_INPUT_S, -1.0f);
        m_InputMapper->AddMapping(InputAction::Interact, kb, KEY_INPUT_F);
    }
    if (id == 0 && mouse) {
        m_InputMapper->AddAxisMapping(InputActionMouse::Wheel, mouse, MouseDevice::AxisID::Wheel);
    }
    //if (pad) {
    //    // ゲームパッド軸
    //    m_InputMapper->AddAxisMapping(InputAction::MoveX, pad, 0);
    //    m_InputMapper->AddAxisMapping(InputAction::MoveY, pad, 1);
    //    m_InputMapper->AddMapping(InputAction::Interact, pad, PAD_INPUT_A);
    //}

}
