#include "EnhancedInputComponent.h"

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
};

MEnhancedInputComponent::MEnhancedInputComponent() : ImplPtr(new Impl()) {}

MEnhancedInputComponent::~MEnhancedInputComponent() { delete ImplPtr; }

void MEnhancedInputComponent::ClearBindings() {
  ImplPtr->Bindings.clear();
  ImplPtr->LastAxis2DValues.clear();
  ImplPtr->LastAxis1DValues.clear();
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
    const InputMapper& mapper, bool AllowUI, bool AllowGame
) {
  std::unordered_map<std::string, FVector2D> currentFrameAxis2D;
  std::unordered_map<std::string, float> currentFrameAxis1D;
  constexpr float EpsilonSq = 0.0001f;

  for (const auto& b : ImplPtr->Bindings) {
    bool is2DAxis = false;
    FVector2D currentAxis2D = FVector2D::ZeroVector;
    float currentAxis1D = mapper.GetAxisValue(b.ActionName);

    if (b.ActionName == InputAction::Move) {
      currentAxis2D = mapper.GetAxis2DValue(InputActionLower::MoveX, InputActionLower::MoveY);
      is2DAxis = true;
    } else if (b.ActionName == UIAction::Move) {
      currentAxis2D = mapper.GetAxis2DValue(UIActionLower::MoveX, UIActionLower::MoveY);
      is2DAxis = true;
    } else if (b.ActionName == InputAction::Look) {
      currentAxis2D = mapper.GetAxis2DValue(InputActionLower::LookX, InputActionLower::LookY);
      is2DAxis = true;
    }

    if (is2DAxis) {
      currentFrameAxis2D[b.ActionName] = currentAxis2D;
    } else {
      currentFrameAxis1D[b.ActionName] = currentAxis1D;
    }
    bool bIsUIAction = b.bIsUIAction;
    if (bIsUIAction && !AllowUI) continue;
    if (!bIsUIAction && !AllowGame) continue;

    bool trigger = false;
    switch (b.Event) {
      case ETriggerEvent::Started:
        if (is2DAxis) {
          const float lastSq = ImplPtr->LastAxis2DValues[b.ActionName].SizeSquared();
          trigger = (lastSq <= EpsilonSq) && (currentAxis2D.SizeSquared() > EpsilonSq);
        } else {
          const bool isButtonStart = mapper.GetPressStart(b.ActionName);
          const bool isAxisStart = (std::abs(ImplPtr->LastAxis1DValues[b.ActionName]) <= EpsilonSq) &&
                                   (std::abs(currentAxis1D) > EpsilonSq);
          trigger = isButtonStart || isAxisStart;
        }
        break;
      case ETriggerEvent::Triggered:
        if (is2DAxis) {
          trigger = (currentAxis2D.SizeSquared() > EpsilonSq);
        } else {
          trigger = mapper.GetPressing(b.ActionName) || (std::abs(currentAxis1D) > EpsilonSq);
        }
        break;
      case ETriggerEvent::Completed:
        if (is2DAxis) {
          const float lastSq = ImplPtr->LastAxis2DValues[b.ActionName].SizeSquared();
          trigger = (lastSq > EpsilonSq) && (currentAxis2D.SizeSquared() <= EpsilonSq);
        } else {
          const bool isButtonRelease = mapper.GetRelease(b.ActionName);
          const bool isAxisRelease = (std::abs(ImplPtr->LastAxis1DValues[b.ActionName]) > EpsilonSq) &&
                                     (std::abs(currentAxis1D) <= EpsilonSq);
          trigger = isButtonRelease || isAxisRelease;
        }
        break;
    }

    if (!trigger || !b.Callback) continue;

    FInputActionValue value;
    value.bIsPressed = true;
    value.Axis1D = currentAxis1D;
    value.Axis2D = currentAxis2D;
    b.Callback(value);
  }
  ImplPtr->LastAxis2DValues = currentFrameAxis2D;
  ImplPtr->LastAxis1DValues = currentFrameAxis1D;
}
