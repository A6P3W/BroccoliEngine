#include "PlayerController.h"
#include <InputMapper.h>
#include <EnhancedInputComponent.h>
APlayerController::APlayerController()
{
    auto InputComp = std::make_unique<MEnhancedInputComponent>();
    m_InputCompPtr = InputComp.get();
    AddComponent(std::move(InputComp));
}

void APlayerController::Possess(APawn* NewPawn)
{
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
        InputComp->ProcessInputBindings();
    }
}
