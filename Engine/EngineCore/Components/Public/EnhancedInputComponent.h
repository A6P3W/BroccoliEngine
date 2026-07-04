#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ActorComponent.h"
#include "InputMapper.h"
#include "UMath.h"

enum class ETriggerEvent { Started, Triggered, Completed };

struct FInputActionValue {
  FVector2D Axis2D = FVector2D::ZeroVector;
  float Axis1D = 0.0f;
  bool bIsPressed = false;
};

class MEnhancedInputComponent : public MActorComponent {
 private:
  struct FInputBinding {
    std::string ActionName;
    ETriggerEvent Event;
    std::function<void(const FInputActionValue&)> Callback;
    bool IsUIAction = false;
  };
  std::vector<FInputBinding> Bindings;
  std::unordered_map<std::string, FVector2D> LastAxis2DValues;
  std::unordered_map<std::string, float> LastAxis1DValues;

 public:
  void ClearBindings() {
    Bindings.clear();
    LastAxis2DValues.clear();
    LastAxis1DValues.clear();
  }
  template <class T>
  void BindAction(
      const std::string& actionName,
      ETriggerEvent event,
      T* obj,
      void (T::*func)(const FInputActionValue&),
      bool isUIAction = false
  ) {
    Bindings.push_back(
        {actionName,
         event,
         [obj, func](const FInputActionValue& v) { (obj->*func)(v); },
         isUIAction}
    );
  }
  template <class T>
  void BindAction(
      const std::string& actionName,
      ETriggerEvent event,
      T* obj,
      void (T::*func)(),
      bool isUIAction = false
  ) {
    Bindings.push_back(
        {actionName, event, [obj, func](const FInputActionValue&) { (obj->*func)(); }, isUIAction}
    );
  }

  void ProcessInputBindings(const InputMapper& mapper, bool AllowUI, bool AllowGame);
};
