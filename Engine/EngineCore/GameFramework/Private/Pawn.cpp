#include "Pawn.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include "Utils/Log.h"
APawn::APawn()
{
	auto camera = std::make_unique<MCameraComponent>();
	m_camera = camera.get();
	AddComponent(std::move(camera));
	m_camera->SetFOV(1);


	auto InputComp = std::make_unique<MEnhancedInputComponent>();
	m_InputCompPtr = InputComp.get();
	AddComponent(std::move(InputComp));


}
void APawn::OnPossesed()
{
	m_camera->SetActiveCamera();
}
void APawn::OnUpdate(float DeltaTime)
{
}

void APawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{	
	PlayerInputComponent->BindAction(E_INPUT_ACTION::INTERACT, ETriggerEvent::Started, this, &APawn::OnInteractPressed);
	PlayerInputComponent->BindAction(E_INPUT_ACTION::MOVE, ETriggerEvent::Triggered, this, &APawn::OnMove);

}

void APawn::OnInteractPressed()
{
	M_LOG("FFFF");
}

void APawn::OnMove(const FInputActionValue& Value)
{
	AddActorWorldOffset(Value.Axis2D*200);
	M_LOG("ActorLocation X:{} Y:{}", GetActorLocation().X, GetActorLocation().Y);
}
