#include "GameModeBase.h"
#include "Pawn.h"
#include "ObjectManager.h"
#include "CameraComponent.h"

AGameModeBase::AGameModeBase()
{
    auto camera = std::make_unique<MCameraComponent>();
    auto* camPtr = camera.get();
    this->AddComponent(std::move(camera)); 
    camPtr->SetActiveCamera();
}

void AGameModeBase::OnUpdate(float DeltaTime)
{
}
