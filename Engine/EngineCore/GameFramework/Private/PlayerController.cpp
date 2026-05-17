#include "PlayerController.h"
#include <InputMapper.h>
#include <EnhancedInputComponent.h>
APlayerController::APlayerController()
{
}

void APlayerController::Possess(APawn* NewPawn)
{
	m_TargetPawn = NewPawn;
    m_TargetPawn->OnPossesed();

    if (m_TargetPawn->GetInputComponent()) {
        m_TargetPawn->SetupPlayerInputComponent(m_TargetPawn->GetInputComponent());
    }
}

void APlayerController::OnUpdate(float DeltaTime)
{
	if (!m_TargetPawn)return;

    if (MEnhancedInputComponent* InputComp = m_TargetPawn->GetInputComponent()) {
        InputComp->ProcessInputBindings();
    }
}
