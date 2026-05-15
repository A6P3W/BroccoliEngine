#include "PlayerController.h"
#include <InputMapper.h>
APlayerController::APlayerController()
{
}

void APlayerController::Possess(APawn* NewPawn)
{
	m_TargetPawn = NewPawn;
    m_TargetPawn->OnPossesed();
}

void APlayerController::OnUpdate(float DeltaTime)
{
	if (!m_TargetPawn)return;

    if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::UP)) {
        m_TargetPawn->FowardBack(1.0f);
    }
    if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::DOWN)) {
        m_TargetPawn->FowardBack(-1.0f);
    }
    if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::LEFT)) {
        m_TargetPawn->LeftRight(1.0f);
    }
    if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::RIGHT)) {
        m_TargetPawn->LeftRight(-1.0f);
    }
}
