#include "PlayerController.h"

#include <EnhancedInputComponent.h>
#include <GamePadDevice.h>
#include <InputDevice.h>
#include <InputManager.h>
#include <InputMapper.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <UIManager.h>

REGISTER_ACTOR(APlayerController)

struct APlayerController::Impl {
  APawn* TargetPawn = nullptr;
  MEnhancedInputComponent* InputComponent = nullptr;
  std::unique_ptr<InputMapper> InputMapperPtr;
  int PlayerId = 0;
  EInputMode InputMode = EInputMode::GameAndUI;
};

APlayerController::APlayerController() : ImplPtr(new Impl()) {
  ImplPtr->InputComponent = NewObject<MEnhancedInputComponent>(this);
  if (ImplPtr->InputComponent) {
    ImplPtr->InputComponent->RegisterComponent();
  }
  ImplPtr->InputMapperPtr = std::make_unique<InputMapper>();
  SetUpdateableAnytime(true);
  SetPlayerId(0);
}

APlayerController::~APlayerController() { delete ImplPtr; }
int APlayerController::GetPlayerId() const { return ImplPtr->PlayerId; }
InputMapper* APlayerController::GetInputMapper() { return ImplPtr->InputMapperPtr.get(); }
MEnhancedInputComponent* APlayerController::GetInputComponent() { return ImplPtr->InputComponent; }
APawn* APlayerController::GetPawn() const { return ImplPtr->TargetPawn; }
void APlayerController::SetInputMode(EInputMode NewInputMode) { ImplPtr->InputMode = NewInputMode; }
EInputMode APlayerController::GetInputMode() const { return ImplPtr->InputMode; }
void APlayerController::Possess(APawn* NewPawn) {
  if (GetInputComponent()) {
    GetInputComponent()->ClearBindings();
  }

  if (ImplPtr->TargetPawn && ImplPtr->TargetPawn != NewPawn) {
    ImplPtr->TargetPawn->OnUnPossessed();
  }

  ImplPtr->TargetPawn = NewPawn;

  if (ImplPtr->TargetPawn) {
    ImplPtr->TargetPawn->OnPossessedBy(this);

    SetupPlayerInputComponent(GetInputComponent());
    if (GetInputComponent()) {
      ImplPtr->TargetPawn->SetupPlayerInputComponent(GetInputComponent());
    }
  }
}

void APlayerController::OnUpdate(float DeltaTime) {
  if (!ImplPtr->TargetPawn) return;

  if (bIsLocallyControlled) {
    if (MEnhancedInputComponent* InputComp = GetInputComponent()) {
      bool bAllowUI =
          (ImplPtr->InputMode == EInputMode::UIOnly || ImplPtr->InputMode == EInputMode::GameAndUI);
      bool bAllowGame =
          (ImplPtr->InputMode == EInputMode::GameOnly ||
           ImplPtr->InputMode == EInputMode::GameAndUI);
      InputComp->ProcessInputBindings(*ImplPtr->InputMapperPtr, bAllowUI, bAllowGame);
    }
  }
}

void APlayerController::SetPlayerId(int id) { ImplPtr->PlayerId = id; }

void APlayerController::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  auto* manager = UIManager::GetInstance();
  PlayerInputComponent->BindAction(
      UIAction::Move, ETriggerEvent::Triggered, manager, &UIManager::Navigate, true
  );
  PlayerInputComponent->BindAction(
      UIAction::Submit, ETriggerEvent::Started, manager, &UIManager::Submit, true
  );
  PlayerInputComponent->BindAction(
      UIAction::Cancel, ETriggerEvent::Started, manager, &UIManager::Cancel, true
  );
}

void APlayerController::SetupInputMappings() {
  ImplPtr->InputMapperPtr->RemoveMapping(InputActionLower::MoveX);
  ImplPtr->InputMapperPtr->RemoveMapping(InputActionLower::MoveY);
  ImplPtr->InputMapperPtr->RemoveMapping(UIActionLower::MoveX);
  ImplPtr->InputMapperPtr->RemoveMapping(UIActionLower::MoveY);
  ImplPtr->InputMapperPtr->RemoveMapping(InputActionMouse::Wheel);
  ImplPtr->InputMapperPtr->RemoveMapping(InputAction::Interact);

  auto& IM = InputManager::GetInstance();
  auto* kb = IM.GetDevice<KeyboardDevice>();
  auto* mouse = IM.GetDevice<MouseDevice>();
  auto* pad = IM.GetDevice<GamepadDevice>();

  if (kb) {
    // ゲーム移動
    ImplPtr->InputMapperPtr->AddMapping(InputActionLower::MoveX, kb, EKey::A, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(InputActionLower::MoveX, kb, EKey::D, "", -1.0f);
    ImplPtr->InputMapperPtr->AddMapping(InputActionLower::MoveY, kb, EKey::W, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(InputActionLower::MoveY, kb, EKey::S, "", -1.0f);

    // UI操作
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, EKey::A, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, EKey::D, "", -1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, EKey::W, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, EKey::S, "", -1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, EKey::Up, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveY, kb, EKey::Down, "", -1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, EKey::Left, "", 1.0f);
    ImplPtr->InputMapperPtr->AddMapping(UIActionLower::MoveX, kb, EKey::Right, "", -1.0f);

    ImplPtr->InputMapperPtr->AddMapping(InputAction::Interact, kb, EKey::F);
    ImplPtr->InputMapperPtr->AddMapping(UIAction::Submit, kb, EKey::Space);
    ImplPtr->InputMapperPtr->AddMapping(UIAction::Submit, kb, EKey::Enter);
    ImplPtr->InputMapperPtr->AddMapping(UIAction::Cancel, kb, EKey::Escape);
    ImplPtr->InputMapperPtr->AddMapping(InputAction::Pause, kb, EKey::Escape);
  }
  if (mouse) {
    ImplPtr->InputMapperPtr->AddMapping(InputActionMouse::MouseLeft, mouse, EMouseButton::Left);
    ImplPtr->InputMapperPtr->AddMapping(InputActionMouse::MouseRight, mouse, EMouseButton::Right);
    ImplPtr->InputMapperPtr->AddAxisMapping(
        InputActionMouse::Wheel, mouse, MouseDevice::AxisID::Wheel
    );
    ImplPtr->InputMapperPtr->AddAxisMapping(
        InputActionLower::LookX, mouse, MouseDevice::AxisID::MouseX
    );
    ImplPtr->InputMapperPtr->AddAxisMapping(
        InputActionLower::LookY, mouse, MouseDevice::AxisID::MouseY
    );
  }
  if (pad) {
    // ゲーム移動
    ImplPtr->InputMapperPtr->AddAxisMapping(
        InputActionLower::MoveX, pad, static_cast<int>(AxisID::LeftX), -1.0f
    );
    ImplPtr->InputMapperPtr->AddAxisMapping(
        InputActionLower::MoveY, pad, static_cast<int>(AxisID::LeftY), 1.0f
    );

    // UI操作
    ImplPtr->InputMapperPtr->AddAxisMapping(
        UIActionLower::MoveX, pad, static_cast<int>(AxisID::LeftX), -1.0f
    );
    ImplPtr->InputMapperPtr->AddAxisMapping(
        UIActionLower::MoveY, pad, static_cast<int>(AxisID::LeftY), 1.0f
    );
    ImplPtr->InputMapperPtr->AddMapping(
        UIActionLower::MoveY, pad, EGamepadButton::DPadUp, "", 1.0f
    );
    ImplPtr->InputMapperPtr->AddMapping(
        UIActionLower::MoveY, pad, EGamepadButton::DPadDown, "", -1.0f
    );
    ImplPtr->InputMapperPtr->AddMapping(
        UIActionLower::MoveX, pad, EGamepadButton::DPadLeft, "", 1.0f
    );
    ImplPtr->InputMapperPtr->AddMapping(
        UIActionLower::MoveX, pad, EGamepadButton::DPadRight, "", -1.0f
    );

    ImplPtr->InputMapperPtr->AddMapping(InputAction::Interact, pad, EGamepadButton::South);
    ImplPtr->InputMapperPtr->AddMapping(UIAction::Submit, pad, EGamepadButton::South);
    ImplPtr->InputMapperPtr->AddMapping(UIAction::Cancel, pad, EGamepadButton::East);
    ImplPtr->InputMapperPtr->AddMapping(InputAction::Pause, pad, EGamepadButton::Start);
  }
}
