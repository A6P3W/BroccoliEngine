#include "EnhancedInputComponent.h"

#include <cmath>
#include <unordered_map>
#include <vector>

struct MEnhancedInputComponent::Impl {
  struct FInputBinding {
    std::string ActionName;
    ETriggerEvent Event;
    std::function<void(const FInputActionValue&)> Callback;
    bool bIsUIAction = false;
  };

  std::vector<FInputBinding> Bindings;
  std::unordered_map<std::string, FVector2D> LastAxis2DValues;
  std::unordered_map<std::string, float> LastAxis1DValues;
  std::unordered_map<std::string, EInputDeviceType> LastSourceDevices;
  bool bIsFirstProcess = true;
};

MEnhancedInputComponent::MEnhancedInputComponent() : ImplPtr(new Impl()) {}

MEnhancedInputComponent::~MEnhancedInputComponent() { delete ImplPtr; }

void MEnhancedInputComponent::ClearBindings() {
  ImplPtr->Bindings.clear();
  ImplPtr->LastAxis2DValues.clear();
  ImplPtr->LastAxis1DValues.clear();
  ImplPtr->LastSourceDevices.clear();
  ImplPtr->bIsFirstProcess = true;
}

void MEnhancedInputComponent::AddBinding(
    const std::string& ActionName,
    ETriggerEvent Event,
    std::function<void(const FInputActionValue&)> Callback,
    bool bIsUIAction
) {
  ImplPtr->Bindings.push_back({ActionName, Event, std::move(Callback), bIsUIAction});
}

void MEnhancedInputComponent::ProcessInputBindings(
    const InputMapper& Mapper, bool AllowUI, bool AllowGame
) {
  std::unordered_map<std::string, FVector2D> CurrentFrameAxis2D;
  std::unordered_map<std::string, float> CurrentFrameAxis1D;
  std::unordered_map<std::string, EInputDeviceType> CurrentFrameSourceDevices;
  constexpr float EpsilonSq = 0.0001f;

  for (const auto& Binding : ImplPtr->Bindings) {
    bool bIs2DAxis = false;
    FVector2D CurrentAxis2D = FVector2D::ZeroVector();
    float CurrentAxis1D = Mapper.GetAxisValue(Binding.ActionName);
    EInputDeviceType CurrentSourceDevice = Mapper.GetAxisValueDevice(Binding.ActionName);

    if (Binding.ActionName == InputAction::Move) {
      CurrentAxis2D = Mapper.GetAxis2DValue(InputActionLower::MoveX, InputActionLower::MoveY);
      CurrentSourceDevice =
          Mapper.GetAxis2DValueDevice(InputActionLower::MoveX, InputActionLower::MoveY);
      bIs2DAxis = true;
    } else if (Binding.ActionName == UIAction::Move) {
      CurrentAxis2D = Mapper.GetAxis2DValue(UIActionLower::MoveX, UIActionLower::MoveY);
      CurrentSourceDevice = Mapper.GetAxis2DValueDevice(UIActionLower::MoveX, UIActionLower::MoveY);
      bIs2DAxis = true;
    } else if (Binding.ActionName == InputAction::Look) {
      CurrentAxis2D = Mapper.GetAxis2DValue(InputActionLower::LookX, InputActionLower::LookY);
      CurrentSourceDevice =
          Mapper.GetAxis2DValueDevice(InputActionLower::LookX, InputActionLower::LookY);
      bIs2DAxis = true;
    }

    if (bIs2DAxis) {
      CurrentFrameAxis2D[Binding.ActionName] = CurrentAxis2D;
    } else {
      CurrentFrameAxis1D[Binding.ActionName] = CurrentAxis1D;
    }
    CurrentFrameSourceDevices[Binding.ActionName] = CurrentSourceDevice;

    if (ImplPtr->bIsFirstProcess) continue;

    if (Binding.bIsUIAction && !AllowUI) continue;
    if (!Binding.bIsUIAction && !AllowGame) continue;

    bool bTrigger = false;
    EInputDeviceType SourceDevice = EInputDeviceType::None;
    switch (Binding.Event) {
      case ETriggerEvent::Started:
        if (bIs2DAxis) {
          const float LastSq = ImplPtr->LastAxis2DValues[Binding.ActionName].SizeSquared();
          bTrigger = LastSq <= EpsilonSq && CurrentAxis2D.SizeSquared() > EpsilonSq;
          SourceDevice = CurrentSourceDevice;
        } else {
          SourceDevice = Mapper.GetPressStartDevice(Binding.ActionName);
          const bool bIsButtonStart = SourceDevice != EInputDeviceType::None;
          const bool bIsAxisStart =
              std::abs(ImplPtr->LastAxis1DValues[Binding.ActionName]) <= EpsilonSq &&
              std::abs(CurrentAxis1D) > EpsilonSq;
          bTrigger = bIsButtonStart || bIsAxisStart;
          if (!bIsButtonStart) SourceDevice = CurrentSourceDevice;
        }
        break;
      case ETriggerEvent::Triggered:
        if (bIs2DAxis) {
          bTrigger = CurrentAxis2D.SizeSquared() > EpsilonSq;
          SourceDevice = CurrentSourceDevice;
        } else {
          SourceDevice = Mapper.GetPressingDevice(Binding.ActionName);
          const bool bIsButtonPressed = SourceDevice != EInputDeviceType::None;
          bTrigger = bIsButtonPressed || std::abs(CurrentAxis1D) > EpsilonSq;
          if (!bIsButtonPressed) SourceDevice = CurrentSourceDevice;
        }
        break;
      case ETriggerEvent::Completed:
        if (bIs2DAxis) {
          const float LastSq = ImplPtr->LastAxis2DValues[Binding.ActionName].SizeSquared();
          bTrigger = LastSq > EpsilonSq && CurrentAxis2D.SizeSquared() <= EpsilonSq;
          SourceDevice = ImplPtr->LastSourceDevices[Binding.ActionName];
        } else {
          SourceDevice = Mapper.GetReleaseDevice(Binding.ActionName);
          const bool bIsButtonRelease = SourceDevice != EInputDeviceType::None;
          const bool bIsAxisRelease =
              std::abs(ImplPtr->LastAxis1DValues[Binding.ActionName]) > EpsilonSq &&
              std::abs(CurrentAxis1D) <= EpsilonSq;
          bTrigger = bIsButtonRelease || bIsAxisRelease;
          if (!bIsButtonRelease) {
            SourceDevice = ImplPtr->LastSourceDevices[Binding.ActionName];
          }
        }
        break;
    }

    if (!bTrigger || !Binding.Callback) continue;

    FInputActionValue Value;
    Value.bIsPressed = true;
    Value.Axis1D = CurrentAxis1D;
    Value.Axis2D = CurrentAxis2D;
    Value.SourceDevice = SourceDevice;
    Binding.Callback(Value);
  }

  ImplPtr->LastAxis2DValues = CurrentFrameAxis2D;
  ImplPtr->LastAxis1DValues = CurrentFrameAxis1D;
  ImplPtr->LastSourceDevices = CurrentFrameSourceDevices;
  ImplPtr->bIsFirstProcess = false;
}
