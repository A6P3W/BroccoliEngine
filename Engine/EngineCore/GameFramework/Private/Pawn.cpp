#include "Pawn.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include <DxLib.h>
APawn::APawn()
{
	auto camera = std::make_unique<MCameraComponent>();
	m_camera = camera.get();
	AddComponent(std::move(camera));
	m_camera->SetFOV(1);
}
void APawn::OnPossesed()
{
}
void APawn::OnUpdate(float DeltaTime)
{
}

void APawn::FowardBack(float Scale)
{
	AddActorWorldOffset({ 0,Scale });
}

void APawn::LeftRight(float Scale)
{
	AddActorWorldOffset({ Scale,0 });
}
