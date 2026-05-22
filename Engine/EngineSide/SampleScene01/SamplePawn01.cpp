#include "SamplePawn01.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include "Utils/Log.h"
#include <CircleCollisionComponent.h>
#include <MovementComponent.h>


ASamplePawn01::ASamplePawn01()
{
	auto col = std::make_unique<MCircleCollisionComponent>(32.0f);
	col->SetParentComponent(GetRootComponent());
	AddComponent(std::move(col));


	auto movement = std::make_unique<MMovementComponent>();
	m_movement = movement.get();
	AddComponent(std::move(movement));
}
void ASamplePawn01::OnPossesed()
{
	m_camera->SetFOV(1);
	m_camera->SetActiveCamera();
}
void ASamplePawn01::OnUpdate(float DeltaTime)
{
}

void ASamplePawn01::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent)
{	
	PlayerInputComponent->BindAction(E_INPUT_ACTION::INTERACT, ETriggerEvent::Started, this, &ASamplePawn01::OnInteractPressed);
	PlayerInputComponent->BindAction(E_INPUT_ACTION::MOVE, ETriggerEvent::Triggered, this, &ASamplePawn01::OnMove);

}

void ASamplePawn01::OnInteractPressed()
{
	M_LOG("F");
}

void ASamplePawn01::OnMove(const FInputActionValue& Value)
{
	m_movement->AddWorldForce(Value.Axis2D * -2);
}
