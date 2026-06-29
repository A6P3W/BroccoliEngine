#include "SamplePawn01.h"

#include <CircleCollisionComponent.h>
#include <DxLib.h>
#include <EnhancedInputComponent.h>
#include <MovementComponent.h>

#include "CameraComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "MainMenu.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "UIManager.h"
#include "World.h"

REGISTER_ACTOR(ASamplePawn01)

ASamplePawn01::ASamplePawn01() {
  auto col = std::make_unique<MCircleCollisionComponent>(32.0f);
  col->SetParentComponent(GetRootComponent());
  AddComponent(std::move(col));

  auto movement = std::make_unique<MMovementComponent>();
  Movement = movement.get();
  AddComponent(std::move(movement));
}
void ASamplePawn01::BeginPlay() {
  auto* mainMenuWidget = GetWorld()->GetObjectManager()->SpawnObject<MainMenu>();

  // 3. UIManagerにPushして画面の最前面で開く
  UIManager::GetInstance()->AddWidget(mainMenuWidget);
}
void ASamplePawn01::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  Camera->SetFOV(1);
}
void ASamplePawn01::OnUpdate(float DeltaTime) {}

void ASamplePawn01::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputAction::Interact, ETriggerEvent::Started, this, &ASamplePawn01::OnInteractPressed
  );
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &ASamplePawn01::OnMove
  );
}

void ASamplePawn01::OnInteractPressed() { M_LOG("F"); }

void ASamplePawn01::OnMove(const FInputActionValue& Value) {
  Movement->AddWorldForce(Value.Axis2D * -2);
}
